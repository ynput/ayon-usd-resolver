"""Optional telemetry: test records to Loki, spans to Tempo.

Nothing here decides a test outcome. Assertions use ayon-tap and memcached counters
directly; this module only publishes the same numbers so runs can be compared over time
in Grafana. It is off unless configured, and a broken backend never fails a test:

- ``RIT_LOKI_URL`` (e.g. ``http://ayon-loki:3100``) enables Loki records.
- ``OTEL_EXPORTER_OTLP_ENDPOINT`` (e.g. ``http://ayon-tempo:4318``) enables traces.

Loki streams all carry ``job="resolver_it"`` plus a ``kind`` label:

- ``session``: one record per resolver process (see ``session_record``)
- ``test``: one record per test outcome
- ``resolver_log``: the resolver's own output for each session, one line per entry

``run_id`` and ``build`` are labels too. Both have one value per run, so cardinality
stays at the number of runs kept by Loki's retention.
"""

from __future__ import annotations

import contextlib
import json
import logging
import os
import time
import urllib.request
import uuid
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from collections.abc import Generator

log = logging.getLogger("resolver-it.telemetry")

SERVICE_NAME = "resolver-it"


class Telemetry:
    def __init__(self, build: str, run_id: str | None = None) -> None:
        self.run_id = run_id or time.strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:6]
        self.build = build
        self.loki_url = (os.environ.get("RIT_LOKI_URL") or "").rstrip("/")
        self._tracer = None
        self._provider = None
        self._propagator = None
        if os.environ.get("OTEL_EXPORTER_OTLP_ENDPOINT"):
            self._init_tracing()

    @property
    def enabled(self) -> bool:
        return bool(self.loki_url or self._tracer)

    def _init_tracing(self) -> None:
        try:
            from opentelemetry.exporter.otlp.proto.http.trace_exporter import (
                OTLPSpanExporter,
            )
            from opentelemetry.sdk.resources import Resource
            from opentelemetry.sdk.trace import TracerProvider
            from opentelemetry.sdk.trace.export import BatchSpanProcessor
            from opentelemetry.trace.propagation.tracecontext import (
                TraceContextTextMapPropagator,
            )
        except ImportError:
            log.warning("OTEL_EXPORTER_OTLP_ENDPOINT set but opentelemetry is not installed; no traces")
            return
        resource = Resource.create({
            "service.name": SERVICE_NAME,
            "rit.run_id": self.run_id,
            "rit.build": self.build,
        })
        self._provider = TracerProvider(resource=resource)
        self._provider.add_span_processor(BatchSpanProcessor(OTLPSpanExporter()))
        self._tracer = self._provider.get_tracer(SERVICE_NAME)
        self._propagator = TraceContextTextMapPropagator()

    # -- traces -------------------------------------------------------------------------

    @contextlib.contextmanager
    def span(self, name: str, **attributes: Any) -> Generator[Any, None, None]:  # ruff: ignore[any-type] - span type depends on whether OTel is installed
        """A span when tracing is on, otherwise ``None``."""
        if not self._tracer:
            yield None
            return
        with self._tracer.start_as_current_span(name) as current:
            for key, value in attributes.items():
                if value is not None:
                    current.set_attribute(key, value)
            yield current

    def traceparent(self) -> str | None:
        """W3C traceparent of the current span, for ayon-tap to parent its spans under."""
        if not self._propagator:
            return None
        carrier: dict[str, str] = {}
        self._propagator.inject(carrier)
        return carrier.get("traceparent")

    @staticmethod
    def trace_id(span: Any) -> str | None:  # ruff: ignore[any-type] - see span()
        if span is None:
            return None
        context = span.get_span_context()
        return f"{context.trace_id:032x}" if context.is_valid else None

    # -- loki ---------------------------------------------------------------------------

    def push(self, kind: str, records: list[dict[str, Any]], **labels: str) -> None:
        """One Loki stream per call; every record becomes one JSON line."""
        if not self.loki_url or not records:
            return
        stream = {"job": "resolver_it", "kind": kind, "run_id": self.run_id, "build": self.build, **labels}
        now = time.time_ns()
        # Distinct, increasing timestamps keep line order inside the stream.
        values = [[str(now + i), json.dumps(record, default=str)] for i, record in enumerate(records)]
        payload = json.dumps({"streams": [{"stream": stream, "values": values}]}).encode()
        request = urllib.request.Request(
            f"{self.loki_url}/loki/api/v1/push",
            data=payload,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        try:
            with urllib.request.urlopen(request, timeout=10):
                pass
        except OSError as exc:
            log.warning("loki push failed (%s records, kind=%s): %s", len(records), kind, exc)

    def shutdown(self) -> None:
        if self._provider:
            self._provider.shutdown()


def stat_deltas(before: dict[str, int], after: dict[str, int]) -> dict[str, int]:
    """Changes of the memcached counters a session is judged by; absent ones are skipped."""
    keys = ("get_hits", "get_misses", "cmd_get", "cmd_set", "delete_hits", "delete_misses", "cmd_flush", "curr_items")
    return {f"mc_{k}": after[k] - before[k] for k in keys if k in before and k in after}
