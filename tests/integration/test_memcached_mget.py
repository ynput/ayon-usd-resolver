"""Prewarm reads memcached with multi-key gets, one per chunk of 256 keys.

memcached's ``stats`` counts keys, not requests, so the request counts come from a proxy
between the resolver and the backend (lib/mcproxy.py).
"""

from __future__ import annotations

import threading
import time
import uuid
from pathlib import Path

from lib.mc import memcached_knobs
from seed.seed_project import layer_with_references, publish_extra, uri_for

CHUNK = 256


def test_warm_prewarm_batches_its_reads(project, backend, run_resolver, mc_proxy) -> None:
    assert run_resolver("cold", **memcached_knobs(backend)).ok
    proxy = mc_proxy()

    warm = run_resolver("warm-prewarm", proxy=proxy, **memcached_knobs(backend))
    assert warm.ok, warm.describe()
    assert warm.resolved == project.expected, warm.describe()
    assert warm.tap["resolve_uris_total"] == 0, warm.tap
    assert warm.mc["mc_get_max_keys"] >= min(CHUNK, len(project.uris) - 2), warm.mc
    assert warm.mc["mc_get_commands"] < len(project.uris) / 2, warm.mc

    single = run_resolver("warm-no-prewarm", proxy=proxy, **memcached_knobs(backend, AYON_RESOLVER_NO_PREWARM="1"))
    assert single.resolved == project.expected, single.describe()
    assert single.mc["mc_get_max_keys"] == 1, single.mc
    assert warm.mc["mc_get_commands"] < single.mc["mc_get_commands"], (warm.mc, single.mc)


def test_more_keys_than_a_chunk_are_split(project, backend, run_resolver, mc_proxy) -> None:
    count = CHUNK + 44
    tag = uuid.uuid4().hex[:8]
    references = [uri_for(project, f"usdNone{tag}{i}", "1") for i in range(count)]
    root = publish_extra(project, f"usdWide{tag}", ["1"], text=layer_with_references(references))["1"]

    proxy = mc_proxy()
    cold = run_resolver("wide-cold", root=root, uris=[], proxy=proxy, **memcached_knobs(backend))
    assert cold.ok, cold.describe()
    assert proxy.get_sizes[CHUNK] == 1, dict(proxy.get_sizes)
    assert proxy.get_sizes[count - CHUNK] == 1, dict(proxy.get_sizes)
    assert max(proxy.get_sizes) == CHUNK, dict(proxy.get_sizes)
    assert set(references) <= set(cold.tap["resolved_uris"]), "every miss must reach AYON"

    warm = run_resolver("wide-warm", root=root, uris=[], proxy=proxy, **memcached_knobs(backend))
    assert warm.ok, warm.describe()
    assert not set(references) & set(warm.tap["resolved_uris"]), "known misses must not ask AYON"


def test_dangling_references_are_read_once(project, backend, run_resolver, mc_proxy) -> None:
    """After prewarm's multi-key read, composing the stage reads no marker again."""
    count = CHUNK + 44
    tag = uuid.uuid4().hex[:8]
    references = [uri_for(project, f"usdNone{tag}{i}", "1") for i in range(count)]
    root = publish_extra(project, f"usdWide{tag}", ["1"], text=layer_with_references(references))["1"]
    knobs = memcached_knobs(backend)
    assert run_resolver("dangling-cold", root=root, uris=[], **knobs).ok

    proxy = mc_proxy()
    warm = run_resolver("dangling-warm", root=root, uris=[], proxy=proxy, **knobs)
    assert warm.ok, warm.describe()
    assert warm.tap["resolve_uris_total"] == 0, warm.tap
    assert warm.mc["mc_get_keys"] <= count + 2, warm.mc
    assert warm.mc["mc_get_commands"] <= 5, warm.mc


def test_one_long_key_does_not_spoil_the_chunk(project, backend, run_resolver, mc_proxy) -> None:
    """libmemcached rejects a whole mget for one over-long key, so it has to be left out."""
    tag = uuid.uuid4().hex[:8]
    assets = [u for u in project.uris if "usdAsset" in u]
    long_uri = uri_for(project, "usdAsset", "1") + "&pad=" + "x" * 260
    root = publish_extra(project, f"usdLongRefs{tag}", ["1"], text=layer_with_references([*assets, long_uri]))["1"]
    knobs = memcached_knobs(backend)
    assert run_resolver("cold", root=root, uris=[], **knobs).ok

    proxy = mc_proxy()
    warm = run_resolver("warm", root=root, uris=[], proxy=proxy, **knobs)
    assert warm.ok, warm.describe()
    assert "memcached mget failed" not in warm.log, warm.describe()
    assert warm.mc["mc_get_max_keys"] >= min(CHUNK, len(assets)), warm.mc
    assert set(warm.tap["resolved_uris"]) <= {long_uri}, warm.tap["resolved_uris"]


def test_one_dead_server_keeps_hits_from_the_others(project, backend, run_resolver, mc_proxy, tmp_path) -> None:
    """A server dying after startup costs its own keys, not those of the healthy server."""
    healthy, dying = mc_proxy(), mc_proxy()
    knobs = memcached_knobs(backend, AYON_MEMCACHED_SERVERS=f"{healthy.address},{dying.address}")

    # Enough keys per server to overflow libmemcached's 8 KiB write buffer, so writes continue after the first
    # one reaches the dead connection.
    tag = uuid.uuid4().hex[:8]
    uris = [uri_for(project, f"usdNone{tag}{i}", "1") for i in range(CHUNK)]
    root = tmp_path / "wide.usda"
    root.write_text(layer_with_references(uris), encoding="utf-8")
    assert run_resolver("fill", root=str(root), uris=[], **knobs).ok
    assert healthy.commands["add"]
    assert dying.commands["add"]
    expected_on_dying = dying.commands["add"]

    signal = tmp_path / "server-dies"

    def kill_server() -> None:
        while not signal.exists():
            time.sleep(0.02)
        dying.close()
        time.sleep(0.2)
        Path(f"{signal}.ack").touch()

    steps = [
        {"op": "resolve", "uris": [str(root)]},  # builds the cache and connects to both servers
        {"op": "handshake", "path": str(signal)},
        {"op": "open", "root": str(root)},
    ]
    killer = threading.Thread(target=kill_server, daemon=True)
    killer.start()
    result = run_resolver("server-dies", steps=steps, **knobs)
    killer.join(timeout=5)

    assert result.ok, result.describe()
    asked = set(result.tap["resolved_uris"])
    assert len(asked) <= expected_on_dying, "keys on the healthy server went to AYON"


def test_failed_mget_falls_through_to_ayon(project, backend, run_resolver, mc_proxy) -> None:
    assert run_resolver("cold", **memcached_knobs(backend)).ok

    proxy = mc_proxy(fail_multi_get=True)
    result = run_resolver("mget-dropped", proxy=proxy, **memcached_knobs(backend))
    assert result.ok, result.describe()
    assert result.resolved == project.expected, result.describe()
    assert result.prims == project.expected_prims, result.describe()
    assert result.mc["mc_failed_gets"] >= 1, result.mc
    assert "memcached mget failed" in result.log, result.describe()
    assert result.tap["resolve_uris_total"] > 0, "the keys mget lost must come from AYON"
