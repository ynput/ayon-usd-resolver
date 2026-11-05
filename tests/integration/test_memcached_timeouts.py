"""A reply that arrives after the client timeout must not be read as the answer to a later command.

The proxy holds the reads of one key past the client timeout. libmemcached leaves the late reply on
the connection, so without a reconnect every later command on it is one reply behind.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest

from lib.mc import memcached_knobs
from seed.seed_project import layer_with_references

if TYPE_CHECKING:
    from pathlib import Path

TIMEOUT_MS = 500

# Which stall went wrong depends on where the late reply lands: some gave URIs another URI's path,
# others left the cache unused for the rest of the process.
HOLDS_MS = [600, 800, 1500, 2500]


def one_by_one(uris: list[str]) -> list[dict]:
    return [{"op": "resolve", "uris": [uri]} for uri in uris]


def wrong_answers(result, uris: list[str], project, first_step: int = 0) -> dict[str, str]:
    answers = {uri: result.steps[first_step + i]["resolved"][uri] for i, uri in enumerate(uris)}
    return {uri: path for uri, path in answers.items() if path != project.expected[uri]}


def knobs_for(backend) -> dict[str, str]:
    return memcached_knobs(backend, AYON_MEMCACHED_TIMEOUT_MS=str(TIMEOUT_MS))


@pytest.mark.parametrize("hold_ms", HOLDS_MS)
def test_late_reply_does_not_reach_another_uri(project, backend, run_resolver, mc_proxy, hold_ms: int) -> None:
    knobs = knobs_for(backend)
    assert run_resolver("cold", **knobs).ok
    uris = sorted(project.uris)

    result = run_resolver(
        "late-reply",
        steps=[*one_by_one(uris), {"op": "resolve", "uris": uris}],
        proxy=mc_proxy(hold_get=(uris[0], hold_ms)),
        AYON_RESOLVER_NO_PREWARM="1",
        **knobs,
    )
    assert result.ok, result.describe()
    assert not wrong_answers(result, uris, project), result.describe()
    assert result.steps[-1]["resolved"] == project.expected, "a wrong path must not stay in the process"
    assert "memcached get failed" in result.log, "the timeout must be visible in the log"


@pytest.mark.parametrize("hold_ms", HOLDS_MS)
def test_cache_keeps_working_after_a_timeout(project, backend, run_resolver, mc_proxy, hold_ms: int) -> None:
    knobs = knobs_for(backend)
    assert run_resolver("cold", **knobs).ok
    uris = sorted(project.uris)

    result = run_resolver(
        "late-reply-cache",
        steps=one_by_one(uris),
        proxy=mc_proxy(hold_get=(uris[0], hold_ms)),
        AYON_RESOLVER_NO_PREWARM="1",
        **knobs,
    )
    assert result.ok, result.describe()
    assert set(result.tap["resolved_uris"]) <= {uris[0]}, "only the URI that timed out may go to AYON"


@pytest.mark.parametrize("hold_ms", HOLDS_MS)
def test_late_multi_get_reply_does_not_reach_a_single_get(
    project, backend, run_resolver, mc_proxy, tmp_path: Path, hold_ms: int
) -> None:
    knobs = knobs_for(backend)
    assert run_resolver("cold", **knobs).ok
    uris = sorted(project.uris)
    batched, single = uris[:12], uris[12:]
    root = tmp_path / "refs.usda"
    root.write_text(layer_with_references(batched), encoding="utf-8")

    result = run_resolver(
        "late-multi-get",
        steps=[{"op": "open", "root": str(root)}, *one_by_one(single)],
        proxy=mc_proxy(hold_get=(batched[0], hold_ms)),
        **knobs,
    )
    assert result.ok, result.describe()
    assert not wrong_answers(result, single, project, first_step=1), result.describe()


def test_late_marker_does_not_hide_another_uri(project, backend, run_resolver, mc_proxy) -> None:
    """An empty marker read for the wrong key would make a published URI look missing."""
    knobs = knobs_for(backend)
    assert run_resolver("cold", **knobs).ok
    uris = sorted(project.uris)
    missing = uris[0].replace("usdAsset", "usdMissingForLateReply")
    assert run_resolver("miss", steps=one_by_one([missing]), **knobs).ok
    assert backend.client().get(missing) == b"", "the marker must be stored"

    result = run_resolver(
        "late-marker",
        steps=one_by_one([missing, *uris]),
        proxy=mc_proxy(hold_get=(missing, 1500)),
        AYON_RESOLVER_NO_PREWARM="1",
        **knobs,
    )
    assert result.ok, result.describe()
    assert not wrong_answers(result, uris, project, first_step=1), result.describe()
