"""Counting reverse proxy in front of the AYON server.

The resolver talks to AYON through this proxy (``AYON_SERVER_URL=http://ayon-tap:5000``) so
tests can assert exactly which requests a scenario caused — "a warm cache makes zero resolve
calls" is otherwise unobservable — and inject faults into individual endpoints.

Control API (never forwarded upstream), all under ``/__tap``:

- ``GET  /__tap/counts``  request counts per ``METHOD /path`` and per resolved URI, the
  number of error responses (``error_count``: every status >= 400, injected or upstream)
  and the bodies of the first upstream error responses (``errors``, capped, diagnostics only)
- ``POST /__tap/reset``   zero all counters. Optional body ``{"traceparent": "..."}``: spans
  for the following requests are parented under that W3C trace context
- ``POST /__tap/fault``   ``{"path_regex": "...", "method": "GET"|null, "status": 500|null,
  "delay_ms": 0, "body": null}`` — every matching request is delayed, then answered with
  ``status`` and ``body`` as JSON (if a status is set) instead of being forwarded
- ``DELETE /__tap/fault`` remove all faults

With ``OTEL_EXPORTER_OTLP_ENDPOINT`` set, every proxied request is exported as a span, so a
test's trace shows each AYON call the resolver made, its latency and any injected fault.
The resolver itself sends no trace context; the test passes it in through ``reset``, which
works because scenarios run one at a time.
"""

from __future__ import annotations

import asyncio
import contextlib
import json
import logging
import os
import re
from collections import Counter
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any

from aiohttp import ClientSession, ClientTimeout, web

if TYPE_CHECKING:
    from collections.abc import Generator

UPSTREAM = os.environ.get("AYON_UPSTREAM_URL", "http://server:5000").rstrip("/")
MAX_ERROR_DETAILS = 20
PORT = int(os.environ.get("TAP_PORT", "5000"))

# Headers that describe one hop, not the message; never copied across the proxy.
HOP_BY_HOP = {
    "connection",
    "keep-alive",
    "proxy-authenticate",
    "proxy-authorization",
    "te",
    "trailers",
    "transfer-encoding",
    "upgrade",
    "host",
    "content-length",
    "content-encoding",
}

log = logging.getLogger("ayon-tap")


class Tracing:
    """OTel spans per request when configured; every method is a no-op otherwise."""

    def __init__(self) -> None:
        self.tracer = None
        self.parent = None
        if not os.environ.get("OTEL_EXPORTER_OTLP_ENDPOINT"):
            return
        from opentelemetry.exporter.otlp.proto.http.trace_exporter import (
            OTLPSpanExporter,
        )
        from opentelemetry.sdk.resources import Resource
        from opentelemetry.sdk.trace import TracerProvider
        from opentelemetry.sdk.trace.export import BatchSpanProcessor
        from opentelemetry.trace.propagation.tracecontext import (
            TraceContextTextMapPropagator,
        )

        self.provider = TracerProvider(resource=Resource.create({"service.name": "ayon-tap"}))
        self.provider.add_span_processor(BatchSpanProcessor(OTLPSpanExporter()))
        self.tracer = self.provider.get_tracer("ayon-tap")
        self.propagator = TraceContextTextMapPropagator()

    def set_parent(self, traceparent: str | None) -> None:
        if self.tracer:
            self.parent = self.propagator.extract({"traceparent": traceparent}) if traceparent else None

    @contextlib.contextmanager
    def span(self, name: str, **attributes: object) -> Generator[Any, None, None]:
        if not self.tracer:
            yield None
            return
        from opentelemetry.trace import SpanKind

        with self.tracer.start_as_current_span(name, context=self.parent, kind=SpanKind.SERVER) as current:
            for key, value in attributes.items():
                current.set_attribute(key, value)
            yield current


@dataclass
class Fault:
    path_regex: re.Pattern[str]
    method: str | None
    status: int | None
    delay_ms: int
    body: object = None

    def matches(self, method: str, path: str) -> bool:
        if self.method and self.method.upper() != method:
            return False
        return bool(self.path_regex.search(path))


class Tap:
    def __init__(self) -> None:
        self.requests: Counter[str] = Counter()
        self.resolved_uris: Counter[str] = Counter()
        self.faults: list[Fault] = []
        # Every response >= 400 sent to the client, injected or upstream.
        self.error_count = 0
        # First upstream error responses, for failure messages: "METHOD path -> status: body".
        # Capped at MAX_ERROR_DETAILS; use error_count for the number of errors.
        self.errors: list[str] = []
        self.session: ClientSession | None = None
        self.tracing = Tracing()

    def reset(self) -> None:
        self.requests.clear()
        self.resolved_uris.clear()
        self.error_count = 0
        self.errors.clear()

    def snapshot(self) -> dict:
        return {
            "requests": dict(self.requests),
            "resolved_uris": dict(self.resolved_uris),
            "resolve_requests": sum(n for k, n in self.requests.items() if k.endswith(" /api/resolve")),
            "resolve_uris_total": sum(self.resolved_uris.values()),
            "error_count": self.error_count,
            "errors": list(self.errors),
        }


def _count_resolve_body(tap: Tap, body: bytes) -> int:
    try:
        payload = json.loads(body)
    except ValueError:
        return 0
    uris = payload.get("uris") or []
    for uri in uris:
        tap.resolved_uris[uri] += 1
    return len(uris)


async def control(request: web.Request) -> web.Response:
    tap: Tap = request.app["tap"]
    action = request.match_info["action"]
    if action == "counts" and request.method == "GET":
        return web.json_response(tap.snapshot())
    if action == "reset" and request.method == "POST":
        spec = await request.json() if request.can_read_body else {}
        tap.reset()
        tap.tracing.set_parent(spec.get("traceparent"))
        return web.json_response({"ok": True})
    if action == "fault" and request.method == "POST":
        spec = await request.json()
        tap.faults.append(
            Fault(
                path_regex=re.compile(spec["path_regex"]),
                method=spec.get("method"),
                status=spec.get("status"),
                delay_ms=int(spec.get("delay_ms") or 0),
                body=spec.get("body"),
            )
        )
        return web.json_response({"ok": True, "faults": len(tap.faults)})
    if action == "fault" and request.method == "DELETE":
        tap.faults.clear()
        return web.json_response({"ok": True})
    return web.json_response({"error": f"unknown control {request.method} {action}"}, status=404)


async def proxy(request: web.Request) -> web.StreamResponse:
    tap: Tap = request.app["tap"]
    path = request.rel_url.path
    tap.requests[f"{request.method} {path}"] += 1
    body = await request.read()
    uris = _count_resolve_body(tap, body) if path == "/api/resolve" else 0

    with tap.tracing.span(
        f"AYON {request.method} {_span_path(path)}",
        **{"http.request.method": request.method, "url.path": path, "ayon.resolve_uris": uris},
    ) as span:
        response = await _forward(tap, request, path, body, span)
        if response.status >= 400:
            tap.error_count += 1
        if span is not None:
            span.set_attribute("http.response.status_code", response.status)
            if response.status >= 400:
                from opentelemetry.trace import Status, StatusCode

                span.set_status(Status(StatusCode.ERROR, f"HTTP {response.status}"))
        return response


def _span_path(path: str) -> str:
    """Span name without the project name, so spans group by endpoint."""
    return re.sub(r"^/api/projects/[^/]+/", "/api/projects/{project}/", path)


async def _forward(
    tap: Tap, request: web.Request, path: str, body: bytes, span: object
) -> web.Response:
    for fault in tap.faults:
        if fault.matches(request.method, path):
            if span is not None:
                span.set_attribute("tap.fault.delay_ms", fault.delay_ms)
                span.set_attribute("tap.fault.status", fault.status or 0)
            if fault.delay_ms:
                await asyncio.sleep(fault.delay_ms / 1000)
            if fault.status is not None:
                body = {"detail": "injected by ayon-tap"} if fault.body is None else fault.body
                return web.json_response(body, status=fault.status)

    headers = {k: v for k, v in request.headers.items() if k.lower() not in HOP_BY_HOP}
    assert tap.session is not None
    async with tap.session.request(
        request.method,
        UPSTREAM + str(request.rel_url),
        headers=headers,
        data=body or None,
        allow_redirects=False,
    ) as upstream:
        payload = await upstream.read()
        if upstream.status >= 400 and len(tap.errors) < MAX_ERROR_DETAILS:
            tap.errors.append(f"{request.method} {path} -> {upstream.status}: {payload[:300].decode(errors='replace')}")
        out_headers = {k: v for k, v in upstream.headers.items() if k.lower() not in HOP_BY_HOP}
        return web.Response(status=upstream.status, headers=out_headers, body=payload)


async def _on_startup(app: web.Application) -> None:  # ruff: ignore[unused-async] - aiohttp requires a coroutine
    app["tap"].session = ClientSession(timeout=ClientTimeout(total=120), auto_decompress=True)


async def _on_cleanup(app: web.Application) -> None:
    if app["tap"].session:
        await app["tap"].session.close()
    if app["tap"].tracing.tracer:
        app["tap"].tracing.provider.shutdown()


def make_app() -> web.Application:
    app = web.Application(client_max_size=64 * 1024 * 1024)
    app["tap"] = Tap()
    app.router.add_route("*", "/__tap/{action}", control)
    app.router.add_route("*", "/{tail:.*}", proxy)
    app.on_startup.append(_on_startup)
    app.on_cleanup.append(_on_cleanup)
    return app


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    log.info("ayon-tap: :%d -> %s", PORT, UPSTREAM)
    web.run_app(make_app(), port=PORT, access_log=None)
