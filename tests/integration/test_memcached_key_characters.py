"""A URI that cannot be a memcached key stays out of memcached.

Whitespace and control characters end the key in the text protocol, and whatever follows would run as a
command: a URI could overwrite another URI's path or flush the cache. Such a URI still resolves through AYON.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest

from lib.mc import memcached_knobs
from seed.seed_project import layer_with_references, uri_for

if TYPE_CHECKING:
    from pathlib import Path

EVIL_PATH = "/evil/path.usda"
QUERY = "?product=usdAsset&version=hero&representation=usd"


def odd_uri(project, kind: str, victim: str) -> str:
    folder = f"ayon://{project.project}/assets/odd"
    return {
        "space": f"{folder} name{QUERY}",
        "tab": f"{folder}\tname{QUERY}",
        "flush": f"{folder}\r\nflush_all\r\nget x{QUERY}",
        "overwrite": f"{folder}\r\nset {victim} 0 0 {len(EVIL_PATH)}\r\n{EVIL_PATH}\r\nget x{QUERY}",
    }[kind]


@pytest.mark.parametrize("kind", ["space", "tab", "flush", "overwrite"])
def test_uri_does_not_reach_memcached(project, backend, run_resolver, mc_proxy, kind: str) -> None:
    knobs = memcached_knobs(backend)
    assert run_resolver("cold", **knobs).ok
    client = backend.client()
    victim = uri_for(project, "usdAsset", "hero")
    uris = sorted(project.uris)
    proxy = mc_proxy()

    result = run_resolver(
        "odd-uri",
        steps=[{"op": "resolve", "uris": [odd_uri(project, kind, victim)]}, *[{"op": "resolve", "uris": [u]} for u in uris]],
        proxy=proxy,
        AYON_RESOLVER_NO_PREWARM="1",
        **knobs,
    )
    assert result.ok, result.describe()
    assert {u: result.resolved[u] for u in uris} == project.expected, result.describe()
    assert result.tap["resolve_requests"] == 1, "only the odd URI goes to AYON; a flush would send them all"
    assert result.mc["mc_get_commands"] == len(uris), "the odd URI must not be sent"
    assert result.mc["mc_add_commands"] == 0, "no miss marker for the odd URI"
    assert " failed:" not in result.log, result.describe()
    stored = client.get(victim)
    assert stored is None or EVIL_PATH.encode() not in stored, stored


def test_prewarm_leaves_the_uri_out_of_its_multi_get(project, backend, run_resolver, mc_proxy, tmp_path: Path) -> None:
    good = uri_for(project, "usdAsset", "1")
    odd = odd_uri(project, "space", good)
    root = tmp_path / "refs.usda"
    root.write_text(layer_with_references([odd, good]), encoding="utf-8")
    proxy = mc_proxy()

    result = run_resolver("odd-prewarm", root=str(root), uris=[good], proxy=proxy, **memcached_knobs(backend))
    assert result.ok, result.describe()
    assert result.resolved[good] == project.published[good], result.describe()
    # The key with a space would otherwise count as two.
    assert result.mc["mc_get_max_keys"] == 1, result.mc
    assert " failed:" not in result.log, result.describe()
