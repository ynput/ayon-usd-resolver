"""A cached path whose root cannot be mapped is a miss, not a result (needs PR #106).

Site roots are fetched once, when the cache is built. If that fetch fails, a warm cache used
to return the stored ``{root[work]}/...`` template as the resolved path.
"""

from __future__ import annotations

import pytest

from lib.mc import memcached_knobs

BAD_KEY = {"AYON_API_KEY": "not-a-valid-key"}
WARNING = "has an unmapped root"


def fill(backend, run_resolver) -> None:
    assert run_resolver("cold", **memcached_knobs(backend)).ok


def test_bad_key_with_warm_cache_fails_cleanly(project, backend, run_resolver) -> None:
    fill(backend, run_resolver)

    bad = run_resolver("bad-key", steps=[{"op": "resolve", "uris": project.uris}], **memcached_knobs(backend, **BAD_KEY))
    assert bad.ok, bad.describe()
    unmapped = [path for path in bad.resolved.values() if "{root[" in path]
    assert not unmapped, f"{len(unmapped)} URIs resolved to an unmapped template, e.g. {unmapped[0]!r}"
    assert bad.resolved == {uri: uri for uri in project.uris}, bad.describe()
    assert set(bad.tap["resolved_uris"]) == set(project.uris), "each URI must fall through to AYON"
    assert bad.log.count(WARNING) == 1, "the warning is once per process"

    good = run_resolver("good-key", steps=[{"op": "resolve", "uris": project.uris}], **memcached_knobs(backend))
    assert good.resolved == project.expected, good.describe()
    assert good.tap["resolve_uris_total"] == 0, "the failed session must not damage the cache"


def test_bad_key_prewarm_queues_the_uris_for_ayon(project, backend, run_resolver) -> None:
    fill(backend, run_resolver)

    bad = run_resolver("bad-key-prewarm", **memcached_knobs(backend, **BAD_KEY))
    assert "{root[" not in str(bad.resolved), bad.describe()
    assert project.root_uri in bad.tap["resolved_uris"], bad.tap["resolved_uris"]
    assert bad.log.count(WARNING) == 1, bad.describe()


@pytest.mark.xfail(
    strict=True,
    reason="PR #106 scope: with a working key AYON's own answer goes through the same empty root map",
)
def test_failed_site_roots_with_a_working_key(project, backend, run_resolver, tap) -> None:
    fill(backend, run_resolver)

    tap.add_fault(r"/siteRoots$", status=500)
    result = run_resolver("no-site-roots", steps=[{"op": "resolve", "uris": project.uris}], **memcached_knobs(backend))
    assert result.resolved == project.expected, result.describe()
