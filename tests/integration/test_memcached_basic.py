"""Scenarios 1-2: memcached as the second-level cache between the resolver and AYON.

1. A cold session fills memcached with every URI of the closure; a warm session in a new
   process composes the same stage without a single AYON resolve.
2. Stored values are rootless (``{root[work]}/...``) and re-root to the expected path.
"""

from __future__ import annotations

import pytest

from lib.mc import memcached_knobs

PREWARM = {
    "prewarm": {},
    "no-prewarm": {"AYON_RESOLVER_NO_PREWARM": "1"},
}


def assert_composed(result, project) -> None:
    assert result.ok, result.describe()
    assert result.resolved == project.expected, result.describe()
    assert result.prims == project.expected_prims, result.describe()


def test_baseline_without_memcached(project, run_resolver) -> None:
    """Precondition for the rest: the resolver composes the seeded closure from AYON alone."""
    result = run_resolver("baseline", AYON_RESOLVER_NO_PREWARM="1")
    assert_composed(result, project)
    assert set(result.tap["resolved_uris"]) == set(project.uris), result.tap


@pytest.mark.parametrize("mode", sorted(PREWARM))
def test_cold_fills_cache_warm_skips_ayon(project, backend, run_resolver, mode: str) -> None:
    knobs = memcached_knobs(backend, **PREWARM[mode])
    client = backend.client()

    cold = run_resolver("cold", **knobs)
    assert_composed(cold, project)
    assert set(cold.tap["resolved_uris"]) == set(project.uris), cold.tap
    not_stored = [uri for uri in project.uris if client.get(uri) is None]
    assert not not_stored, f"{len(not_stored)}/{len(project.uris)} URIs not in {backend.name}: {not_stored[:3]}"

    before = client.stats()
    warm = run_resolver("warm", **knobs)
    after = client.stats()
    assert_composed(warm, project)
    assert warm.tap["resolve_uris_total"] == 0, f"warm session still asked AYON: {warm.tap['resolved_uris']}"
    if "get_hits" in before and "get_hits" in after:  # Dragonfly reports no hit counters
        hits = after["get_hits"] - before["get_hits"]
        assert hits >= len(project.uris), f"get_hits +{hits} for {len(project.uris)} URIs"


def test_cached_values_are_rootless(project, backend, run_resolver) -> None:
    cold = run_resolver("cold", **memcached_knobs(backend, AYON_RESOLVER_NO_PREWARM="1"))
    assert_composed(cold, project)

    client = backend.client()
    for uri, expected in project.expected.items():
        raw = client.get(uri)
        assert raw is not None, f"{uri} not stored in {backend.name}"
        value = raw.decode()
        assert value.startswith("{root[work]}/"), f"{uri} stored as {value!r}, expected a rootless template"
        assert value.replace("{root[work]}", project.work_root, 1) == expected
