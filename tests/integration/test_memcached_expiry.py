"""Expiry per URI class, miss markers and ClearCache without flush_all (needs PR #104).

Each session is a new process, so the only state shared between sessions is memcached:
what these tests read back from it is what another machine would see.
"""

from __future__ import annotations

import threading
import time
import uuid

import pytest

from lib.mc import memcached_knobs
from seed.seed_project import publish_extra, uri_for

MEMCACHED_MAX_RELATIVE_TTL = 2592000
EMPTY_MARKER = b""


def unique(prefix: str = "usdExtra") -> str:
    return f"{prefix}{uuid.uuid4().hex[:8]}"


def resolve(*uris: str) -> list[dict]:
    return [{"op": "resolve", "uris": list(uris)}]


def asked(result, uri: str) -> int:
    """How many times this session sent ``uri`` to AYON."""
    return result.tap["resolved_uris"].get(uri, 0)


def assert_ttl(client, uri: str, low: int, high: int) -> None:
    """Remaining TTL is within [low, high]; only backends with the meta protocol report it."""
    if client.supports_ttl():
        ttl = client.ttl(uri)
        assert ttl is not None, f"{uri} not stored"
        assert low <= ttl <= high, f"TTL {ttl} outside [{low}, {high}] for {uri}"


# -- expiry per URI class -----------------------------------------------------------------


def test_default_ttls(project, backend, run_resolver) -> None:
    """Integers never expire, hero lasts an hour, latest never touches memcached."""
    client = backend.client()
    assert run_resolver("cold", **memcached_knobs(backend)).ok
    for uri in project.expected:
        assert client.get(uri) is not None, f"{uri} not stored"
        if "version=hero" in uri:
            assert_ttl(client, uri, 3500, 3600)
        else:
            assert_ttl(client, uri, -1, -1)

    latest = uri_for(project, "usdAsset", "latest")
    result = run_resolver("latest", steps=resolve(latest), **memcached_knobs(backend))
    assert result.resolved[latest] == project.published[uri_for(project, "usdAsset", "2")], result.describe()
    assert client.get(latest) is None


@pytest.mark.parametrize(
    ("knob", "value", "version", "low", "high"),
    [
        ("AYON_MEMCACHED_EXPIRE_SECONDS", "100", "1", 90, 100),
        ("AYON_MEMCACHED_HERO_EXPIRE_SECONDS", "120", "hero", 110, 120),
        ("AYON_MEMCACHED_HERO_EXPIRE_SECONDS", "0", "hero", -1, -1),
        ("AYON_MEMCACHED_LATEST_EXPIRE_SECONDS", "140", "latest", 130, 140),
    ],
)
def test_ttl_settings(project, backend, run_resolver, knob, value, version, low, high) -> None:
    uri = uri_for(project, "usdAsset", version)
    result = run_resolver("ttl", steps=resolve(uri), **memcached_knobs(backend, **{knob: value}))
    assert result.ok, result.describe()
    assert backend.client().get(uri) is not None, f"{uri} not stored"
    assert_ttl(backend.client(), uri, low, high)


def test_hero_can_bypass_memcached(project, backend, run_resolver) -> None:
    uri = uri_for(project, "usdAsset", "hero")
    knobs = memcached_knobs(backend, AYON_MEMCACHED_HERO_EXPIRE_SECONDS="-1")
    for _ in range(2):
        result = run_resolver("hero-bypass", steps=resolve(uri), **knobs)
        assert result.resolved[uri] == project.published[uri], result.describe()
        assert asked(result, uri) == 1, "a bypassed class must be resolved by AYON every session"
    assert backend.client().get(uri) is None


def test_ttl_above_memcached_limit_is_clamped(project, backend, run_resolver) -> None:
    """Above 30 days memcached reads the value as an absolute time, which expires instantly."""
    uri = uri_for(project, "usdAsset", "1")
    knobs = memcached_knobs(backend, AYON_MEMCACHED_EXPIRE_SECONDS=str(MEMCACHED_MAX_RELATIVE_TTL + 1))
    result = run_resolver("clamped", steps=resolve(uri), **knobs)
    assert "30 day limit" in result.log, result.describe()
    assert backend.client().get(uri) is not None
    assert_ttl(backend.client(), uri, MEMCACHED_MAX_RELATIVE_TTL - 60, MEMCACHED_MAX_RELATIVE_TTL)


def test_invalid_ttl_falls_back_to_default(project, backend, run_resolver) -> None:
    uri = uri_for(project, "usdAsset", "1")
    result = run_resolver("invalid", steps=resolve(uri), **memcached_knobs(backend, AYON_MEMCACHED_EXPIRE_SECONDS="soon"))
    assert "invalid expiry 'soon'" in result.log, result.describe()
    assert_ttl(backend.client(), uri, -1, -1)


def test_short_ttl_expires_on_the_server(project, backend, run_resolver) -> None:
    uri = uri_for(project, "usdAsset", "1")
    knobs = memcached_knobs(backend, AYON_MEMCACHED_EXPIRE_SECONDS="4")
    assert run_resolver("set", steps=resolve(uri), **knobs).ok
    assert backend.client().get(uri) is not None
    time.sleep(5.5)
    assert backend.client().get(uri) is None
    assert asked(run_resolver("after-expiry", steps=resolve(uri), **knobs), uri) == 1


@pytest.mark.parametrize(
    ("version", "stored", "ttl"),
    [("v1", True, -1), ("HERO", True, None), ("latestdone", False, None), ("*", False, None)],
)
def test_version_classification(project, backend, run_resolver, version, stored, ttl) -> None:
    """Versions normalise like the server does; only an integer is immutable."""
    uri = uri_for(project, "usdAsset", version)
    assert run_resolver("classify", steps=resolve(uri), **memcached_knobs(backend)).ok
    assert (backend.client().get(uri) is not None) is stored
    if ttl is not None:
        assert_ttl(backend.client(), uri, ttl, ttl)


def test_uri_without_version_is_not_cached(project, backend, run_resolver) -> None:
    uri = uri_for(project, "usdAsset", "1").replace("version=1&", "")
    assert run_resolver("no-version", steps=resolve(uri), **memcached_knobs(backend)).ok
    assert backend.client().get(uri) is None


def test_latest_stays_live_by_default(project, backend, run_resolver) -> None:
    product = unique()
    latest = uri_for(project, product, "latest")
    knobs = memcached_knobs(backend)
    v1 = publish_extra(project, product, ["1"])["1"]
    first = run_resolver("latest-v1", steps=resolve(latest), **knobs)
    assert first.resolved[latest] == project.published[v1], first.describe()

    v2 = publish_extra(project, product, ["2"])["2"]
    second = run_resolver("latest-v2", steps=resolve(latest), **knobs)
    assert second.resolved[latest] == project.published[v2], second.describe()
    assert backend.client().get(latest) is None


def test_latest_with_a_ttl_is_served_from_memcached(project, backend, run_resolver) -> None:
    product = unique()
    latest = uri_for(project, product, "latest")
    knobs = memcached_knobs(backend, AYON_MEMCACHED_LATEST_EXPIRE_SECONDS="60")
    v1 = publish_extra(project, product, ["1"])["1"]
    assert run_resolver("fill", steps=resolve(latest), **knobs).resolved[latest] == project.published[v1]

    publish_extra(project, product, ["2"])
    warm = run_resolver("warm", steps=resolve(latest), **knobs)
    assert warm.resolved[latest] == project.published[v1], "the configured TTL allows this staleness"
    assert asked(warm, latest) == 0


# -- miss markers -------------------------------------------------------------------------


def test_miss_is_stored_as_an_empty_marker(project, backend, run_resolver) -> None:
    missing = uri_for(project, unique("usdMissing"), "1")
    client = backend.client()

    uncached = run_resolver("uncached", steps=resolve(missing), AYON_RESOLVER_NO_PREWARM="1")
    knobs = memcached_knobs(backend)
    first = run_resolver("miss", steps=resolve(missing), **knobs)
    assert asked(first, missing) == 1
    assert client.get(missing) == EMPTY_MARKER
    assert_ttl(client, missing, 50, 60)

    second = run_resolver("known-miss", steps=resolve(missing), **knobs)
    assert asked(second, missing) == 0, "a known miss must not ask AYON"
    assert second.resolved[missing] == uncached.resolved[missing], "a known miss differs from a failed resolve"
    assert "\x01" not in second.resolved[missing]


def test_miss_caching_can_be_disabled(project, backend, run_resolver) -> None:
    missing = uri_for(project, unique("usdMissing"), "1")
    knobs = memcached_knobs(backend, AYON_MEMCACHED_MISS_EXPIRE_SECONDS="-1")
    for _ in range(2):
        assert asked(run_resolver("miss-off", steps=resolve(missing), **knobs), missing) == 1
    assert backend.client().get(missing) is None


def test_zero_miss_ttl_is_refused(project, backend, run_resolver) -> None:
    missing = uri_for(project, unique("usdMissing"), "1")
    result = run_resolver("miss-zero", steps=resolve(missing), **memcached_knobs(backend, AYON_MEMCACHED_MISS_EXPIRE_SECONDS="0"))
    assert "a miss must expire" in result.log, result.describe()
    assert backend.client().get(missing) == EMPTY_MARKER
    assert_ttl(backend.client(), missing, 50, 60)


def test_marker_expiry_reveals_a_publish(project, backend, run_resolver) -> None:
    product = unique("usdLate")
    missing = uri_for(project, product, "1")
    knobs = memcached_knobs(backend, AYON_MEMCACHED_MISS_EXPIRE_SECONDS="1")
    assert asked(run_resolver("miss", steps=resolve(missing), **knobs), missing) == 1
    publish_extra(project, product, ["1"])
    time.sleep(2.5)
    found = run_resolver("published", steps=resolve(missing), **knobs)
    assert found.resolved[missing] == project.published[missing], found.describe()


@pytest.mark.parametrize(
    ("version", "knobs", "marker", "low", "high"),
    [
        ("hero", {"AYON_MEMCACHED_HERO_EXPIRE_SECONDS": "30"}, True, 20, 30),
        ("hero", {"AYON_MEMCACHED_HERO_EXPIRE_SECONDS": "-1"}, False, 0, 0),
        ("latest", {}, False, 0, 0),
    ],
)
def test_marker_never_outlives_its_class(project, backend, run_resolver, version, knobs, marker, low, high) -> None:
    missing = uri_for(project, unique("usdMissing"), version)
    assert run_resolver("miss", steps=resolve(missing), **memcached_knobs(backend, **knobs)).ok
    assert (backend.client().get(missing) is not None) is marker
    if marker:
        assert_ttl(backend.client(), missing, low, high)


def test_ayon_failure_writes_no_marker(project, backend, run_resolver, tap) -> None:
    missing = uri_for(project, unique("usdMissing"), "1")
    tap.add_fault(r"^/api/resolve$", status=500)
    result = run_resolver("server-error", steps=resolve(missing), **memcached_knobs(backend))
    assert result.tap["error_count"] >= 1, result.describe()
    assert backend.client().get(missing) is None


def test_marker_never_overwrites_a_stored_path(project, backend, run_resolver, tap) -> None:
    """A path another process stores while this one waits for AYON is kept."""
    uri = uri_for(project, unique("usdMissing"), "1")
    path = b"{root[work]}/stored/by/another/process.usda"
    tap.add_fault(r"^/api/resolve$", delay_ms=4000)
    timer = threading.Timer(2.5, backend.client().set, args=(uri, path))
    timer.start()
    try:
        result = run_resolver("race", steps=resolve(uri), **memcached_knobs(backend))
    finally:
        timer.join()
    assert asked(result, uri) == 1, "the write must land while the AYON request is in flight"
    assert backend.client().get(uri) == path


def test_delete_from_cache_drops_a_marker(project, backend, run_resolver) -> None:
    product = unique("usdLate")
    uri = uri_for(project, product, "1")
    knobs = memcached_knobs(backend)
    assert run_resolver("miss", steps=resolve(uri), **knobs).ok
    publish_extra(project, product, ["1"])

    result = run_resolver(
        "delete", steps=[*resolve(uri), {"op": "delete", "uri": uri}, *resolve(uri)], **knobs
    )
    before, after = result.steps[0]["resolved"][uri], result.steps[2]["resolved"][uri]
    assert before != project.published[uri], "the marker should still hide the publish"
    assert after == project.published[uri], result.describe()
    assert backend.client().get(uri) not in {None, EMPTY_MARKER}


@pytest.mark.parametrize("operation", [{"op": "clear_cache"}, "delete"])
def test_marker_delete_keeps_a_path_stored_meanwhile(project, backend, run_resolver, operation) -> None:
    uri = uri_for(project, unique("usdMissing"), "1")
    path = "{root[work]}/stored/by/another/process.usda"
    knobs = memcached_knobs(backend)
    assert run_resolver("miss", steps=resolve(uri), **knobs).ok
    operation = {"op": "delete", "uri": uri} if operation == "delete" else operation
    other_client = {"op": "mc_set", "host": backend.host, "port": backend.port, "key": uri, "value": path}
    result = run_resolver("keep", steps=[*resolve(uri), other_client, operation], **knobs)
    assert result.ok, result.describe()
    assert backend.client().get(uri) == path.encode()


# -- ClearCache ---------------------------------------------------------------------------


def test_clear_cache_never_flushes(project, backend, run_resolver) -> None:
    """Hero and marker keys this process used are deleted. Integer keys and others' keys stay."""
    client = backend.client()
    missing = uri_for(project, unique("usdMissing"), "1")
    bystander = f"rit-bystander-{uuid.uuid4().hex[:8]}"
    client.set(bystander, b"value")
    integers = [u for u in project.uris if "version=hero" not in u]
    heroes = [u for u in project.uris if "version=hero" in u]
    knobs = memcached_knobs(backend)
    assert heroes
    assert integers
    assert run_resolver("fill-marker", steps=resolve(missing), **knobs).ok

    result = run_resolver("clear", steps=[*resolve(*project.uris, missing), {"op": "clear_cache"}], **knobs)
    assert result.ok, result.describe()
    assert client.get(bystander) == b"value", "ClearCache touched a key it does not own"
    assert all(client.get(u) is not None for u in integers), "integer keys must survive ClearCache"
    assert all(client.get(u) is None for u in heroes), "hero keys must be deleted"
    assert client.get(missing) is None, "the marker this process saw must be deleted"
    if "cmd_flush" in result.mc:
        assert result.mc["cmd_flush"] == 0
        assert result.mc["delete_hits"] == len(heroes) + 1


def test_clear_cache_skips_a_bypassed_class(project, backend, run_resolver) -> None:
    uris = [uri_for(project, "usdAsset", "latest"), uri_for(project, "usdAsset", "hero")]
    knobs = memcached_knobs(backend, AYON_MEMCACHED_HERO_EXPIRE_SECONDS="-1")
    result = run_resolver("clear-bypass", steps=[*resolve(*uris), {"op": "clear_cache"}], **knobs)
    assert result.ok, result.describe()
    if "delete_hits" in result.mc:
        assert result.mc["delete_hits"] + result.mc.get("delete_misses", 0) == 0, result.mc


def test_clear_cache_reasks_within_the_process(project, backend, run_resolver) -> None:
    uri = uri_for(project, "usdAsset", "hero")
    result = run_resolver("clear-reask", steps=[*resolve(uri), {"op": "clear_cache"}, *resolve(uri)], **memcached_knobs(backend))
    assert result.steps[2]["resolved"][uri] == project.published[uri], result.describe()
    assert asked(result, uri) == 2, "the deleted hero key has to come from AYON again"


# -- key limit ----------------------------------------------------------------------------


def test_uri_over_the_key_limit_is_skipped_quietly(project, backend, run_resolver) -> None:
    uri = uri_for(project, "usdAsset", "1") + "&pad=" + "x" * 260
    assert len(uri) > 250
    knobs = memcached_knobs(backend)
    for _ in range(2):
        result = run_resolver("long-key", steps=resolve(uri), **knobs)
        assert result.ok, result.describe()
        assert asked(result, uri) == 1, "an uncacheable URI goes to AYON every session"
        assert "memcached get failed" not in result.log, result.describe()
        assert "memcached set failed" not in result.log, result.describe()
