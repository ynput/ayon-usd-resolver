"""An answer from before an invalidation must not be stored after it, with no Python in the process.

Most hosts call the resolver from C++, so nothing stops an invalidation from landing while prewarm's
batched lookup is waiting. A Python caller may not show this: scenarios run through lib/probe.py with
prewarm on passed with or without the fix. These scenarios use cpp/cpp_probe.cpp instead.

The first group holds the AYON answer. The second holds a value on its way back from memcached, after
removing it there. Each runs through prewarm's batched lookup and through a single lookup. libpython is
loaded because this USD build links it, but nothing initializes an interpreter.
"""

from __future__ import annotations

import subprocess
import threading
import time
from pathlib import Path
from typing import TYPE_CHECKING

import pytest

from lib.mc import memcached_knobs
from seed.seed_project import layer_with_references, uri_for

if TYPE_CHECKING:
    from collections.abc import Callable

SOURCE = Path(__file__).with_name("cpp") / "cpp_probe.cpp"
RESOLVER_LIB = "/opt/resolver/ayonUsdResolver/lib"
USD_LIB = "/opt/usd/lib"
STALE_PATH = "/stale/answer/from/before/the/invalidation.usda"
HOLD_MS = 800
ROUNDS = 40
HAMMER_THREADS = 8
PATHS = {"prewarm": {}, "single": {"AYON_RESOLVER_NO_PREWARM": "1"}}


@pytest.fixture(scope="session")
def cpp_probe(tmp_path_factory: pytest.TempPathFactory) -> Path:
    binary = tmp_path_factory.mktemp("cpp_probe") / "cpp_probe"
    subprocess.run(
        [
            "g++", "-std=c++17", "-O0", "-w", "-pthread",
            "-I/opt/usd/include", "-I/usr/include/python3.9",
            str(SOURCE), "-o", str(binary),
            f"-L{USD_LIB}", f"-L{RESOLVER_LIB}",
            f"-Wl,-rpath-link,{USD_LIB}:{RESOLVER_LIB}", "-Wl,--no-as-needed",
            "-l:ayonUsdResolver.so", "-lusd_usd", "-lusd_ar", "-lusd_sdf", "-lusd_js", "-lusd_tf", "-lpython3.9",
        ],
        check=True,
        timeout=600,
    )
    return binary


def answer_when(path: Path, ready: Callable[[], bool], then: Callable[[], None] = lambda: None) -> None:
    """Acknowledge the probe's handshake at ``path`` once ``ready`` is true, after running ``then``."""

    def wait() -> None:
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            if path.exists() and ready():
                then()
                Path(f"{path}.ack").touch()
                return
            time.sleep(0.01)

    threading.Thread(target=wait, daemon=True).start()


def write_root(tmp_path: Path, uri: str) -> str:
    root = tmp_path / "refs.usda"
    root.write_text(layer_with_references([uri]), encoding="utf-8")
    return str(root)


def invalidation(operation: str, uri: str) -> dict:
    return {"op": "delete", "uri": uri} if operation == "delete" else {"op": operation}


def run_during_ayon_answer(project, backend, run_resolver, tap, cpp_probe, tmp_path, entities, operation, path):
    uri = uri_for(project, "usdAsset", "hero")
    tap.add_fault(r"^/api/resolve$", status=200, body=[{"uri": uri, "entities": entities}], delay_ms=HOLD_MS, times=1)
    held = tmp_path / "held"
    answer_when(held, lambda: tap.counts()["requests"].get("POST /api/resolve", 0) > 0)
    result = run_resolver(
        "stale-answer-cpp",
        steps=[
            {"op": "open_in_background", "root": write_root(tmp_path, uri)},
            {"op": "handshake", "path": str(held)},
            invalidation(operation, uri),
            {"op": "join"},
            {"op": "resolve", "uris": [uri]},
        ],
        command=[str(cpp_probe)],
        **PATHS[path],
        **memcached_knobs(backend),
    )
    assert result.ok, result.describe()
    assert result.steps[2]["open_finished"] is False, "the invalidation has to land during the open"
    if path == "prewarm":
        assert "batchResolvePathSerial(1 uris)" in result.log, result.describe()
    return uri, result


@pytest.mark.parametrize("path", sorted(PATHS))
@pytest.mark.parametrize("operation", ["clear_cache", "delete"])
def test_stale_path_from_ayon_is_not_stored(project, backend, run_resolver, tap, cpp_probe, tmp_path, operation, path) -> None:
    uri, result = run_during_ayon_answer(
        project, backend, run_resolver, tap, cpp_probe, tmp_path, [{"filePath": STALE_PATH}], operation, path
    )
    assert result.steps[-1]["resolved"][uri] == project.published[uri], result.describe()
    stored = backend.client().get(uri)
    assert stored is None or STALE_PATH.encode() not in stored, stored


@pytest.mark.parametrize("path", sorted(PATHS))
@pytest.mark.parametrize("operation", ["clear_cache", "delete"])
def test_stale_miss_from_ayon_is_not_stored(project, backend, run_resolver, tap, cpp_probe, tmp_path, operation, path) -> None:
    uri, result = run_during_ayon_answer(project, backend, run_resolver, tap, cpp_probe, tmp_path, [], operation, path)
    assert result.steps[-1]["resolved"][uri] == project.published[uri], result.describe()
    assert backend.client().get(uri) != b"", "a marker from before the invalidation must not be stored"


def test_lookups_during_clear_cache_do_not_restore_an_old_path(
    project, backend, run_resolver, cpp_probe, tmp_path
) -> None:
    """ClearCache empties the in-process caches first and deletes from memcached after that.

    A lookup in between starts after the invalidation began, and can still read a path ClearCache has not
    deleted yet. That path must not stay in the process once ClearCache has finished. The window is short and
    this cannot hold it open, so the scenario repeats it with several threads looking up at once.
    """
    uris = sorted(uri for uri in project.uris if "version=hero" in uri)
    client = backend.client()

    def seed() -> None:
        for uri in uris:
            client.set(uri, STALE_PATH.encode())

    steps = []
    for round_number in range(ROUNDS):
        seeded = tmp_path / f"seeded{round_number}"
        answer_when(seeded, lambda: True, seed)
        steps += [
            {"op": "clear_cache"},
            {"op": "handshake", "path": str(seeded)},
            {"op": "resolve", "uris": uris},
            {"op": "hammer_start", "uris": uris, "threads": HAMMER_THREADS},
            {"op": "clear_cache"},
            {"op": "hammer_stop"},
            {"op": "resolve", "uris": uris},
        ]
    result = run_resolver(
        "lookups-during-clear-cpp",
        steps=steps,
        command=[str(cpp_probe)],
        AYON_RESOLVER_NO_PREWARM="1",
        **memcached_knobs(backend, AYON_MEMCACHED_TIMEOUT_MS="5000"),
    )
    assert result.ok, result.describe()
    expected = {uri: project.published[uri] for uri in uris}
    resolves = [step["resolved"] for step in result.steps if "resolved" in step]
    assert all(after_seed == dict.fromkeys(uris, STALE_PATH) for after_seed in resolves[0::2]), "each round starts stale"
    wrong = sum(after_clear != expected for after_clear in resolves[1::2])
    assert not wrong, f"{wrong} of {ROUNDS} rounds kept an old path after ClearCache"


@pytest.mark.parametrize("path", sorted(PATHS))
@pytest.mark.parametrize("operation", ["clear_cache", "delete"])
@pytest.mark.parametrize("value", [STALE_PATH.encode(), b""], ids=["path", "marker"])
def test_stale_memcached_read_is_not_kept(
    project, backend, run_resolver, mc_proxy, cpp_probe, tmp_path, value: bytes, operation, path
) -> None:
    """The entry is gone from memcached by then, so only a kept answer can stop later lookups reaching AYON."""
    uri = uri_for(project, "usdAsset", "hero")
    client = backend.client()
    client.set(uri, value)
    proxy = mc_proxy(hold_value=(uri, HOLD_MS))
    held = tmp_path / "held"
    answer_when(held, proxy.held_values.is_set, client.flush)
    result = run_resolver(
        "stale-read-cpp",
        steps=[
            {"op": "open_in_background", "root": write_root(tmp_path, uri)},
            {"op": "handshake", "path": str(held)},
            invalidation(operation, uri),
            {"op": "join"},
            {"op": "resolve", "uris": [uri]},
        ],
        command=[str(cpp_probe)],
        proxy=proxy,
        **PATHS[path],
        **memcached_knobs(backend, AYON_MEMCACHED_TIMEOUT_MS="5000"),
    )
    assert result.ok, result.describe()
    assert result.steps[-1]["resolved"][uri] == project.published[uri], result.describe()
