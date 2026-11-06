"""Without AYON settings only ayon:// URIs stop resolving; plain paths and stages keep working."""

from __future__ import annotations

from typing import TYPE_CHECKING

from lib.mc import memcached_knobs
from seed.seed_project import layer_with_references, uri_for

if TYPE_CHECKING:
    from pathlib import Path

NO_AYON = {"AYON_API_KEY": None, "AYON_SERVER_URL": None, "AYON_PROJECT_NAME": None}
WARNING = "ayon:// URIs will not resolve in this process"


def test_plain_layer_opens(run_resolver, tmp_path: Path) -> None:
    root = tmp_path / "plain.usda"
    root.write_text('#usda 1.0\n\ndef Xform "plain"\n{\n}\n', encoding="utf-8")
    result = run_resolver("plain", root=str(root), uris=[], **NO_AYON)
    assert result.ok, result.describe()
    assert result.prims == 1, result.describe()
    assert result.log.count(WARNING) == 1, result.describe()
    assert not result.tap["requests"], result.tap


def test_uri_stays_unresolved(project, run_resolver) -> None:
    uri = uri_for(project, "usdAsset", "1")
    result = run_resolver("unresolved", steps=[{"op": "resolve", "uris": [uri]}], **NO_AYON)
    assert result.ok, result.describe()
    assert result.resolved == {uri: uri}, result.describe()
    assert not result.tap["requests"], result.tap


def test_warm_cache_gives_no_template_and_writes_nothing(project, backend, run_resolver, tmp_path: Path) -> None:
    """Site roots come from AYON, so without it a cached path cannot be mapped."""
    knobs = memcached_knobs(backend)
    assert run_resolver("cold", **knobs).ok
    client = backend.client()
    before = {uri: client.get(uri) for uri in project.uris}
    missing = uri_for(project, "usdNeverPublishedNoAyon", "1")

    result = run_resolver(
        "no-ayon-warm", steps=[{"op": "resolve", "uris": [*project.uris, missing]}], **{**knobs, **NO_AYON}
    )
    assert result.ok, result.describe()
    assert result.resolved == {uri: uri for uri in [*project.uris, missing]}, result.describe()
    assert {uri: client.get(uri) for uri in project.uris} == before, "the shared cache must not change"
    assert client.get(missing) is None, "no AYON answer, so no miss marker"


def test_stage_with_a_reference_opens(project, backend, run_resolver, tmp_path: Path) -> None:
    """Goes through prewarm's batched lookup as well."""
    missing = uri_for(project, "usdNeverPublishedNoAyon", "1")
    root = tmp_path / "refs.usda"
    root.write_text(layer_with_references([missing]), encoding="utf-8")
    result = run_resolver("no-ayon-stage", root=str(root), uris=[], **{**memcached_knobs(backend), **NO_AYON})
    assert result.ok, result.describe()
    assert backend.client().get(missing) is None, "no AYON answer, so no miss marker"
