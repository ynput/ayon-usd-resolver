"""Prewarm reads memcached with multi-key gets, one per chunk of 256 keys (needs PR #105).

memcached's ``stats`` counts keys, not requests, so the request counts come from a proxy
between the resolver and the backend (lib/mcproxy.py).
"""

from __future__ import annotations

import uuid

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
    assert warm.mc["mc_get_max_keys"] >= len(project.uris) - 2, warm.mc
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
    assert warm.mc["mc_get_max_keys"] >= len(assets), warm.mc
    assert set(warm.tap["resolved_uris"]) <= {long_uri}, warm.tap["resolved_uris"]


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
