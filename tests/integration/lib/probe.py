"""One resolver session, run in a fresh interpreter by ``proc.run_probe``.

A fresh process per scenario is required, not a convenience: the memcached and prewarm
gates are read once per process, and every resolve fills the in-process caches. Reusing
an interpreter would measure those caches, not the configuration under test.

Opens the stage (the composition path, including prewarm when enabled), then resolves
every URI in the closure. The resolves after the open are answered from the in-process
cache, so they add no AYON or memcached traffic; they only report the paths.

Writes JSON to ``--out``. stdout is unusable for results: the resolver prints there.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
import traceback


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True, help="root URI to open")
    parser.add_argument("--uris", required=True, help="JSON file: URIs to report resolved paths for")
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    result: dict = {"ok": False}
    try:  # ruff: ignore[too-many-statements-in-try-clause] - one guarded session; any failure is reported the same way
        from pxr import Ar, Usd

        result["resolver"] = str(Ar.GetUnderlyingResolver())
        with open(args.uris, encoding="utf-8") as fh:
            uris = json.load(fh)

        start = time.perf_counter()
        stage = Usd.Stage.Open(args.root)
        result["open_seconds"] = time.perf_counter() - start
        result["prims"] = sum(1 for _ in stage.Traverse()) if stage else 0

        resolver = Ar.GetResolver()
        result["resolved"] = {uri: str(resolver.Resolve(uri)) for uri in uris}
        result["ok"] = True
    except Exception:  # ruff: ignore[blind-except] - reported to the test, which owns the failure message
        result["error"] = traceback.format_exc()

    with open(args.out, "w", encoding="utf-8") as fh:
        json.dump(result, fh, indent=2)
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
