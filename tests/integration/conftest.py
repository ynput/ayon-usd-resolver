"""Fixtures for resolver integration tests against a live AYON server and memcached.

Runs inside the `runner` container (see compose.yml); every fixture reads its endpoints
from the environment that compose sets. Outside that container the whole directory skips.
"""

from __future__ import annotations

import os
from typing import TYPE_CHECKING

import pytest

from lib.mc import Backend, parse_backends
from lib.proc import ProbeResult, run_probe
from lib.tap import TapClient
from lib.telemetry import Telemetry, stat_deltas

if TYPE_CHECKING:
    from collections.abc import Callable, Iterator
    from pathlib import Path

REQUIRED_ENV = ("AYON_SERVER_URL", "AYON_API_KEY", "RIT_TAP_URL", "RIT_BACKENDS")
_missing = [name for name in REQUIRED_ENV if not os.environ.get(name)]
if _missing:
    pytest.skip(
        f"integration env not set ({', '.join(_missing)}); run via tests/integration/compose.yml",
        allow_module_level=True,
    )

BACKENDS = parse_backends(os.environ["RIT_BACKENDS"])
_TELEMETRY: Telemetry | None = None


@pytest.fixture(scope="session")
def tap() -> TapClient:
    return TapClient(os.environ["RIT_TAP_URL"])


@pytest.fixture(scope="session")
def project():  # ruff: ignore[missing-return-type-undocumented-public-function] - Manifest import is deferred until the env check passed
    """Seed one throw-away project for the session; delete it afterwards."""
    from pytest_ayon import register_site

    from seed.seed_project import connect, seed, teardown

    # /api/resolve answers 400 "Site not found" for a site id the server has never seen.
    register_site(*connect(), os.environ.get("AYON_SITE_ID", "resolver-it"), "linux", version="resolver-it")
    manifest = seed(
        os.environ.get("RIT_WORK_ROOT", "/data/work"),
        int(os.environ.get("RIT_ASSET_COUNT", "24")),
    )
    yield manifest
    if os.environ.get("RIT_KEEP_PROJECT", "0") not in {"1", "true"}:
        teardown(manifest)


@pytest.fixture(params=sorted(BACKENDS))
def backend(request: pytest.FixtureRequest) -> Backend:
    """Each memcached-compatible backend, flushed so every test starts cold."""
    selected = BACKENDS[request.param]
    selected.client().flush()
    return selected


@pytest.fixture(scope="session", autouse=True)  # ruff: ignore[pytest-fixture-autouse] - every test publishes
def telemetry() -> Iterator[Telemetry]:
    """Session span plus Loki/Tempo publishing; inert unless configured (lib/telemetry.py)."""
    global _TELEMETRY  # ruff: ignore[global-statement] - pytest hooks have no fixture access
    current = Telemetry(build=os.environ.get("RIT_BUILD", "local"))
    _TELEMETRY = current
    with current.span("pytest.session", **{"rit.run_id": current.run_id}):
        yield current
    current.shutdown()
    _TELEMETRY = None


@pytest.fixture(autouse=True)  # ruff: ignore[pytest-fixture-autouse] - one span per test
def _test_span(request: pytest.FixtureRequest, telemetry: Telemetry) -> Iterator[None]:
    with telemetry.span("test", **{"rit.test": request.node.nodeid}) as span:
        request.node.rit_span = span
        yield


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item: pytest.Item, call: pytest.CallInfo) -> Iterator[None]:
    outcome = yield
    report = outcome.get_result()
    if _TELEMETRY is None or not (report.when == "call" or report.failed):
        return
    span = getattr(item, "rit_span", None)
    if span is not None:
        span.set_attribute("rit.outcome", report.outcome)
        if report.failed:
            from opentelemetry.trace import Status, StatusCode

            span.set_status(Status(StatusCode.ERROR, f"{report.when} failed"))
    _TELEMETRY.push(
        "test",
        [{
            "test": item.nodeid,
            "name": item.name,
            "when": report.when,
            "outcome": report.outcome,
            "duration": round(report.duration, 3),
            "trace_id": Telemetry.trace_id(span),
            "failure": str(report.longrepr)[:4000] if report.failed else None,
        }],
    )


def _backend_for(knobs: dict[str, str]) -> Backend | None:
    if knobs.get("AYON_MEMCACHED_ENABLED") != "true":
        return None
    address = knobs.get("AYON_MEMCACHED_SERVERS", "")
    return next((b for b in BACKENDS.values() if b.address == address), None)


@pytest.fixture
def run_resolver(
    request: pytest.FixtureRequest,
    project,
    tap: TapClient,
    telemetry: Telemetry,
    tmp_path: Path,
) -> Iterator[Callable[..., ProbeResult]]:
    """Run one resolver session in a fresh process; attach the AYON calls it made.

    ``result.tap`` holds the ayon-tap counters for that session alone, ``result.mc`` the
    memcached counter changes (empty without memcached, or on a backend without counters).
    """
    base_env = {
        "AYON_SERVER_URL": os.environ["AYON_SERVER_URL"],
        "AYON_API_KEY": os.environ["AYON_API_KEY"],
        "AYON_SITE_ID": os.environ.get("AYON_SITE_ID", "resolver-it"),
        "AYON_PROJECT_NAME": project.project,
    }

    def run(name: str, **knobs: str) -> ProbeResult:
        backend = _backend_for(knobs)
        client = backend.client() if backend else None
        prewarm = "off" if knobs.get("AYON_RESOLVER_NO_PREWARM") == "1" else "on"
        with telemetry.span(
            f"resolver.session {name}",
            **{"rit.session": name, "rit.backend": backend.name if backend else "none", "rit.prewarm": prewarm},
        ) as span:
            tap.reset(traceparent=telemetry.traceparent())
            before = client.stats() if client else {}
            result = run_probe(
                root=project.root_uri,
                uris=project.uris,
                base_env=base_env,
                knobs=knobs,
                workdir=tmp_path,
                name=name,
            )
            result.tap = tap.counts()
            tap.reset()  # detach: later traffic (seeding, teardown) is not this session's
            result.mc = stat_deltas(before, client.stats()) if client else {}
            record = session_record(
                request.node.nodeid, session=name, backend=backend, prewarm=prewarm, project=project, result=result
            )
            record["trace_id"] = Telemetry.trace_id(span)
            if span is not None:
                for key, value in record.items():
                    if isinstance(value, (bool, int, float, str)):
                        span.set_attribute(f"rit.{key}", value)
        labels = {"backend": record["backend"], "session": name}
        telemetry.push("session", [record], **labels)
        telemetry.push(
            "resolver_log",
            [{"line": line, "test": record["test"], "trace_id": record["trace_id"]} for line in result.log.splitlines()],
            **labels,
        )
        return result

    tap.clear_faults()
    yield run
    tap.clear_faults()


def session_record(
    test: str,
    *,
    session: str,
    backend: Backend | None,
    prewarm: str,
    project,
    result: ProbeResult,
) -> dict:
    """The numbers one resolver session is judged by, flat for LogQL ``| json``."""
    requests = result.tap.get("requests", {})
    return {
        "test": test,
        "session": session,
        "backend": backend.name if backend else "none",
        "prewarm": prewarm,
        "ok": result.ok,
        "composed": result.ok and result.resolved == project.expected and result.prims == project.expected_prims,
        "open_ms": round(result.data.get("open_seconds", 0) * 1000, 1),
        "prims": result.prims,
        "uris": len(project.uris),
        "ayon_requests": sum(requests.values()),
        "ayon_resolve_requests": result.tap.get("resolve_requests", 0),
        "ayon_resolve_uris": result.tap.get("resolve_uris_total", 0),
        "ayon_site_roots": sum(n for k, n in requests.items() if k.endswith("/siteRoots")),
        "ayon_errors": len(result.tap.get("errors", [])),
        **result.mc,
    }
