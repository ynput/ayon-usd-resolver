"""An answer from before an invalidation must not be stored after it.

ayon-tap holds one AYON answer until ClearCache or DeleteFromCache has run, then delivers a stale
answer. A later lookup in the same process has to reach the real server.

Prewarm is off: scenarios through its batched lookup passed with or without the fix, so they showed
nothing. test_stale_answers_without_python.py races batchWarm from C++.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest

from lib.mc import memcached_knobs
from seed.seed_project import layer_with_references, uri_for

if TYPE_CHECKING:
    from pathlib import Path

STALE_PATH = "/stale/answer/from/before/the/invalidation.usda"
ANSWER_DELAY_MS = 800


def run_with_a_stale_answer(project, backend, run_resolver, tap, tmp_path: Path, uri: str, entities, operation):
    tap.add_fault(
        r"^/api/resolve$", status=200, body=[{"uri": uri, "entities": entities}], delay_ms=ANSWER_DELAY_MS, times=1
    )
    root = tmp_path / "refs.usda"
    root.write_text(layer_with_references([uri]), encoding="utf-8")
    invalidate = {"op": "delete", "uri": uri} if operation == "delete" else {"op": operation}
    result = run_resolver(
        "stale-answer",
        steps=[
            {"op": "open_in_background", "root": str(root)},
            {"op": "wait_for_ayon_request", "request": "POST /api/resolve"},
            invalidate,
            {"op": "join"},
            {"op": "resolve", "uris": [uri]},
        ],
        AYON_RESOLVER_NO_PREWARM="1",
        **memcached_knobs(backend),
    )
    assert result.ok, result.describe()
    return result


@pytest.mark.parametrize("operation", ["clear_cache", "delete"])
def test_stale_path_is_not_stored(project, backend, run_resolver, tap, tmp_path, operation) -> None:
    uri = uri_for(project, "usdAsset", "hero")
    result = run_with_a_stale_answer(
        project, backend, run_resolver, tap, tmp_path, uri, [{"filePath": STALE_PATH}], operation
    )
    assert result.steps[-1]["resolved"][uri] == project.published[uri], result.describe()
    stored = backend.client().get(uri)
    assert stored is None or STALE_PATH.encode() not in stored, stored


@pytest.mark.parametrize("operation", ["clear_cache", "delete"])
def test_stale_miss_is_not_stored(project, backend, run_resolver, tap, tmp_path, operation) -> None:
    uri = uri_for(project, "usdAsset", "hero")
    result = run_with_a_stale_answer(project, backend, run_resolver, tap, tmp_path, uri, [], operation)
    assert result.steps[-1]["resolved"][uri] == project.published[uri], result.describe()
    assert backend.client().get(uri) != b"", "a marker from before the invalidation must not be stored"
