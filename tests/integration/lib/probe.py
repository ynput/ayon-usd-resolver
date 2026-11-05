"""One resolver session, run in a fresh interpreter by ``proc.run_probe``.

A fresh process per scenario is required, not a convenience: the memcached and prewarm
gates are read once per process, and every resolve fills the in-process caches. Reusing
an interpreter would measure those caches, not the configuration under test.

Opens the stage (the composition path, including prewarm when enabled), then resolves
every URI in the closure. The resolves after the open are answered from the in-process
cache, so they add no AYON or memcached traffic; they only report the paths.

With ``--steps``, a session runs a JSON list of operations instead of opening a stage, for
behaviour one process must see in sequence: ``resolve`` (``uris``), ``open`` (``root``),
``open_in_background`` (``root``) and ``join`` to wait for it,
``handshake`` (``path``: create that file, then wait for ``path.ack`` from the test), ``clear_cache``, ``delete``
(``uri``), ``sleep`` (``seconds``), and ``mc_set`` (``host``, ``port``, ``key``, ``value``), which
plays another client writing to memcached. ``steps`` in the output has one entry per operation; the paths from
``resolve`` operations are also merged into ``resolved``.

Writes JSON to ``--out``. stdout is unusable for results: the resolver prints there.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import threading
import time
import traceback
import urllib.request
from pathlib import Path


def _run_steps(steps: list[dict]) -> tuple[list[dict], dict[str, str]]:
    from pxr import Ar, Usd
    from usdAssetResolver import AyonUsdResolver

    resolver = Ar.GetResolver()
    context = AyonUsdResolver.ResolverContext()
    outcome: list[dict] = []
    merged: dict[str, str] = {}
    background: threading.Thread | None = None
    for step in steps:
        op = step["op"]
        if op == "resolve":
            paths = {uri: str(resolver.Resolve(uri)) for uri in step["uris"]}
            merged.update(paths)
            outcome.append({"op": op, "resolved": paths})
            continue
        if op == "open":
            Usd.Stage.Open(step["root"])
        elif op == "open_in_background":
            # Composition drops the GIL, so its worker threads resolve while this thread runs later steps.
            background = threading.Thread(target=Usd.Stage.Open, args=(step["root"],))
            background.start()
        elif op == "join":
            background.join()
        elif op == "wait_for_ayon_request":
            while not _ayon_request_seen(step["request"]):
                time.sleep(0.02)
        elif op == "handshake":
            Path(step["path"]).touch()
            ack = Path(step["path"] + ".ack")
            while not ack.exists():
                time.sleep(0.02)
        elif op == "clear_cache":
            context.ClearCache()
        elif op == "delete":
            context.DeleteFromCache(step["uri"])
        elif op == "sleep":
            time.sleep(step["seconds"])
        elif op == "mc_set":
            from mc import TextClient

            TextClient(step["host"], step["port"]).set(step["key"], step["value"].encode())
        else:
            raise ValueError(f"unknown step {op!r}")
        outcome.append({"op": op})
    return outcome, merged


def _ayon_request_seen(request: str) -> bool:
    with urllib.request.urlopen(f"{os.environ['RIT_TAP_URL']}/counts", timeout=10) as response:
        return json.load(response)["requests"].get(request, 0) > 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", help="root URI to open")
    parser.add_argument("--uris", help="JSON file: URIs to report resolved paths for")
    parser.add_argument("--steps", help="JSON file: operations to run instead of opening a stage")
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    result: dict = {"ok": False}
    try:  # ruff: ignore[too-many-statements-in-try-clause] - one guarded session; any failure is reported the same way
        from pxr import Ar, Usd

        result["resolver"] = str(Ar.GetUnderlyingResolver())
        if args.steps:
            with open(args.steps, encoding="utf-8") as fh:
                result["steps"], result["resolved"] = _run_steps(json.load(fh))
        else:
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
