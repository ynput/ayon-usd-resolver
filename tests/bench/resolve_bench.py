r"""AYON USD resolver latency harness — prewarm x memcached x RTT matrix.

Measures the resolver configurations against each other. Both features are env-gated,
so the whole matrix runs against ONE build:

    prewarm   -> AYON_RESOLVER_NO_PREWARM / AYON_RESOLVER_PREWARM
    memcached -> AYON_MEMCACHED_ENABLED=true (needs AYON_MEMCACHED_SERVERS)
    pinning   -> AYON_USD_RESOLVER_ENABLE_PINNING (needs AYON_USD_RESOLVER_PINNING_FILE)

Pinning is a THIRD configuration, not a variant of the other two. In static (pinning) mode
the cache builds NO AyonApi at all, so getAsset() returns the pin-file lookup and nothing
else: no PreCache, no memcached, no /api/resolve fall-through, and batchWarm() no-ops. It
therefore needs no server and cannot be measured against the RTT axis in the usual sense --
its whole point is that the RTT column should not move it. --pin-file supplies the file;
--verify-uri MUST be one of its keys, or the pinned cells cannot resolve anything.

Two roles in one file:

    matrix  - orchestrator. Spawns one `probe` subprocess per cell per probe kind with
              the cell's env, flushes memcached between cold cells, writes CSV.
    probe   - runs INSIDE the DCC interpreter. Times ONE configuration, ONE probe kind,
              and prints JSON to stdout.

A fresh process per cell is not optional: the resolver reads both gates once per process
(prewarm caches its gate in a function-local static, memcached connects at cache
construction), so flipping env inside a live process measures nothing. Likewise ONE probe
kind per process: stage open populates the in-process PreCache, after which a resolve
probe would time PreCache hits rather than the configured backing path.

The env must come from the launcher, never hand-assembled:

    ayon --headless addon applications extractenvironments env.json \
        --project P --folder /path --task T --app houdini/21-0-700-core
    python3 ayon_env_run.py env.json <hython> resolve_bench.py matrix ...

Intended for two cases: a multi-site studio measuring from each site against its own
cache, or a single developer with a local AYON server simulating a faraway one.

RTT axis: a local AYON server answers in well under a millisecond, and with no latency
a cache has nothing to save -- so measuring only against a local server says nothing
about whether the cache is worth deploying against a remote one. --rtt-ms
(repeatable) sweeps synthetic latency through an ALREADY-RUNNING toxiproxy fronting the
server: per level the harness deletes any stale `rtt` toxic (a crashed run must not
poison the level), installs a downstream latency toxic, then MEASURES a request through
the proxy and aborts unless >= 80% of the requested latency materialised — a row
labelled rtt_ms=150 with no latency applied is worse than no data. The whole cell matrix
runs per level, then the toxic is deleted again (also on failure). Probes only route
through the proxy when --ayon-proxy-url rewrites their AYON_SERVER_URL, so --rtt-ms
without it is refused. The rtt_ms CSV column is '' when the sweep is off (latency
unmanaged), the level otherwise (0 = through the proxy, no toxic).

The `multi` probe opens MANY shot stages in ONE process, in order: artists open many
shot USDs per Houdini/Maya session, so later shots hit the shared in-process cache (and
memcached) for assets earlier shots already resolved. Roots come from --roots-file
(JSON list of {"level", "uri"} entries; an entry carrying "uris" is the multi-shot root
set) and/or repeated --root. Each stage open is timed individually plus the total; a
single empty stage fails the cell, same rule as the stage probe.

Aborts (failed warm-up, composed=false, unverified RTT) first write the rows collected
so far to --out — the exit message marks the CSV as partial.

Usage:

    # collect the URI closure of a shot (approximates the prewarm pass's BFS)
    <hython> resolve_bench.py collect --root ayon+entity://PROJ/... \
        --verify-uri 'ayon+entity://PROJ/path?product=usdAsset&version=v001&representation=usd' \
        --out uris.txt

    # run the matrix
    <hython-wrapper> resolve_bench.py matrix --hython <interpreter> \
        --root ayon+entity://PROJ/... --uris uris.txt \
        --verify-uri 'ayon+entity://PROJ/path?product=usdAsset&version=v001&representation=usd' \
        --memcached localhost:11211 --out results.csv

    # sweep RTT and add the multi-shot probe
    <hython-wrapper> resolve_bench.py matrix --hython <interpreter> \
        --root ayon+entity://PROJ/... --uris uris.txt --roots-file roots.json \
        --verify-uri '...' --memcached localhost:11211 \
        --rtt-ms 0 --rtt-ms 15 --rtt-ms 150 \
        --ayon-proxy-url http://toxiproxy:15000 --toxiproxy-api http://toxiproxy:8474 \
        --out results.csv

--verify-uri must name a URI that actually EXISTS on the target server: the AYON resolver
is a primary ArResolver (it registers no URI scheme), so the only honest proof it works is
resolving a real URI to a real, different path. Required unless --allow-no-resolver.

Stdlib only (no pymemcache/typer) so it runs unmodified inside a DCC interpreter.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import pathlib
import socket
import statistics
import subprocess
import sys
import time
import urllib.error
import urllib.request
from typing import TYPE_CHECKING, Any, NoReturn

if TYPE_CHECKING:
    from collections.abc import Callable
    from types import ModuleType

PREWARM_OFF_VALUES = {"0", "false"}

# Blackhole endpoint for the 6-degraded cell. Must be UNROUTABLE, not merely closed: a
# closed port answers ECONNREFUSED instantly and measures nothing, while an unroutable
# address leaves libmemcached's connect/poll hanging for the full AYON_MEMCACHED_TIMEOUT_MS
# on every lookup — the failure mode the cell exists to price. Override: --degraded-server.
# Unroutable by design: a connect here must HANG (packets dropped), not be refused, or the
# "cache configured but dead" cell measures a fast failure instead of a real timeout. Verify
# on your network -- a host that answers ICMP-unreachable makes this cell meaningless. Override
# with --degraded-server.
DEGRADED_MEMCACHED_SERVER = (
    "192.0.2.1:11211"  # RFC 5737 TEST-NET-1: routes nowhere, never refuses
)

# Static-mode gate. NB the resolver treats ANY value but the literal string "false" as ON,
# so "0" enables pinning -- always spell the off state "false".
STAT_FIELDS = 3  # "STAT <name> <value>"
HTTP_NOT_FOUND = 404
PINNING_GATE = "AYON_USD_RESOLVER_ENABLE_PINNING"
PINNING_FILE = "AYON_USD_RESOLVER_PINNING_FILE"

# --- memcached admin (raw protocol; avoids a pymemcache dep inside the DCC) ----


def _memcached_cmd(server: str, command: str, terminator: bytes) -> str:
    """Send one text-protocol command, read until terminator. '' on any failure.

    Returns:
        The server's reply text, or `""` on any failure.
    """
    host, _, port = server.partition(":")
    try:  # noqa: PLW0717 - the whole request is the unit that can fail
        with socket.create_connection(
            (host, int(port or 11211)), timeout=5
        ) as sock:
            sock.sendall(command.encode() + b"\r\n")
            chunks: list[bytes] = []
            while terminator not in b"".join(chunks[-2:]):
                chunk = sock.recv(4096)
                if not chunk:
                    break
                chunks.append(chunk)
            return b"".join(chunks).decode(errors="replace")
    except (OSError, ValueError) as exc:
        print(f"[warn] memcached {server}: {command}: {exc}", file=sys.stderr)
        return ""


def memcached_flush(server: str) -> bool:
    """Flush every entry. Returns False if the server could not be reached.

    DESTRUCTIVE and server-wide: never point this at an instance anything else uses,
    it erases every client's keys. Bench against a dedicated instance.

    Returns:
        True if every entry was dropped, False if the server was unreachable.
    """
    return "OK" in _memcached_cmd(server, "flush_all", b"OK")


def memcached_stats(server: str) -> dict[str, str]:
    """Return the server's `stats` map ({} when unreachable)."""
    raw = _memcached_cmd(server, "stats", b"END")
    stats: dict[str, str] = {}
    for line in raw.splitlines():
        parts = line.split()
        if len(parts) == STAT_FIELDS and parts[0] == "STAT":
            stats[parts[1]] = parts[2]
    return stats


# --- toxiproxy admin (raw HTTP; no client dep inside the DCC) -------------------

# One well-known toxic name so a crashed run's leftover can always be found and removed.
RTT_TOXIC = "rtt"


def _require_http(url: str) -> None:
    """Reject anything but http/https before it reaches urlopen.

    Raises:
        ValueError: If the URL uses any other scheme.
    """
    if not url.startswith(("http://", "https://")):
        msg = f"refusing non-http URL: {url}"
        raise ValueError(msg)


def _toxiproxy_call(
    api: str,
    path: str,
    method: str = "GET",
    payload: dict[str, Any] | None = None,
) -> None:
    """One toxiproxy API call; raises urllib.error.* / OSError on any failure."""
    data = json.dumps(payload).encode() if payload is not None else None
    url = f"{api.rstrip('/')}{path}"
    _require_http(url)
    request = urllib.request.Request(url, data=data, method=method)  # noqa: S310 - scheme checked above
    if data is not None:
        request.add_header("Content-Type", "application/json")
    with urllib.request.urlopen(request, timeout=10):  # noqa: S310 - scheme checked above
        pass


def toxiproxy_delete_toxic(api: str, proxy: str) -> None:
    """Remove the rtt toxic; a toxic that is not there is already the desired state.

    Raises:
        HTTPError: If the proxy rejects the delete for any reason other than 404.
    """
    try:
        _toxiproxy_call(
            api, f"/proxies/{proxy}/toxics/{RTT_TOXIC}", method="DELETE"
        )
    except urllib.error.HTTPError as exc:
        if exc.code != HTTP_NOT_FOUND:
            raise


def toxiproxy_create_toxic(api: str, proxy: str, latency_ms: int) -> None:
    """Install the downstream latency toxic. Callers MUST verify it took effect (measure_proxy_rtt)."""
    payload = {
        "name": RTT_TOXIC,
        "type": "latency",
        "stream": "downstream",
        "toxicity": 1.0,
        "attributes": {"latency": latency_ms, "jitter": 0},
    }
    _toxiproxy_call(
        api, f"/proxies/{proxy}/toxics", method="POST", payload=payload
    )


def measure_proxy_rtt(proxy_url: str, timeout_s: float) -> float:
    """Wall-clock ms for one GET through the proxy to AYON's /api/info.

    The only honest proof a toxic applies: toxiproxy's API accepting the POST says nothing
    about whether the traffic the probes are about to send actually crosses that proxy.

    Returns:
        Round-trip time in milliseconds.
    """
    url = f"{proxy_url.rstrip('/')}/api/info"
    start = time.perf_counter_ns()
    _require_http(url)
    with urllib.request.urlopen(url, timeout=timeout_s) as response:  # noqa: S310 - scheme checked above
        response.read()
    return (time.perf_counter_ns() - start) / 1e6


# --- multi-shot roots ------------------------------------------------------------


def load_multi_roots(path: str) -> list[str]:
    """Ordered multi-shot roots from a roots.json: a list of {"level", "uri" | "uris"} objects.

    An entry carrying "uris" is the multi-shot root set (all such entries concatenate, in
    order); with none present every "uri" entry becomes a root instead.

    Returns:
        The root URIs, ordered by level.
    """
    try:
        with pathlib.Path(path).open(encoding="utf-8") as handle:
            entries = json.load(handle)
    except (OSError, json.JSONDecodeError) as exc:
        sys.exit(f"cannot read roots file {path}: {exc}")
    if not isinstance(entries, list):
        sys.exit(
            f"{path}: expected a JSON list of {{level, uri|uris}} objects"
        )
    multi = [
        uri
        for entry in entries
        if isinstance(entry, dict)
        for uri in entry.get("uris", [])
    ]
    if not multi:
        multi = [
            entry["uri"]
            for entry in entries
            if isinstance(entry, dict) and "uri" in entry
        ]
    return [str(uri) for uri in multi]


# --- probe side (needs pxr) ----------------------------------------------------


def _require_pxr() -> tuple[ModuleType, ModuleType, ModuleType]:
    try:
        from pxr import Ar, Sdf, Usd
    except ImportError:
        sys.exit(
            "probe/collect need a USD-enabled interpreter (hython, mayapy, ...)"
        )
    return Ar, Sdf, Usd


def _registered_uri_schemes() -> list[str]:
    """Registered URI schemes, or [] on USD too old to expose them.

    DIAGNOSTIC ONLY — never a pass/fail criterion. The AYON resolver's plugInfo.json
    declares a *primary* ArResolver ("bases": ["ArResolver"]), not a URI resolver, so a
    fully working resolver never appears here: on a live studio this still returns only
    Houdini's own schemes (hpr/op/opdatablock/opdef/oplib). Gating on this list refuses
    every legitimate run — the check is a real resolve (see ayon_resolver_loaded).

    Returns:
        The registered schemes, or `[]` on a USD too old to expose them.
    """
    Ar, _, _ = _require_pxr()  # noqa: N806 - pxr module objects; lowercase would misname them
    try:
        return sorted(Ar.GetRegisteredURISchemes())
    except AttributeError:
        return []


def prewarm_enabled_from_env() -> bool:
    """Mirror resolver.cpp's gate parsing exactly — any value but 0/false disables.

    Returns:
        True when prewarm is on for this process.
    """
    off = os.environ.get("AYON_RESOLVER_NO_PREWARM")
    if off is not None and off not in PREWARM_OFF_VALUES:
        return False
    return os.environ.get("AYON_RESOLVER_PREWARM") not in PREWARM_OFF_VALUES


def resolver_plugin_path() -> str:
    """Which ayonUsdResolver actually answered — provenance for every CSV row.

    Scheme registration only proves *a* resolver claims ayon://, not that it is the build
    under test. Without this a matrix run against the wrong .so looks identical.

    Returns:
        Filesystem path of the resolver that answered, or `""` if none did.
    """
    try:
        from pxr import Plug

        for plugin in Plug.Registry().GetAllPlugins():
            if "ayonusdresolver" in plugin.name.lower().replace("_", ""):
                return str(plugin.path)
    except Exception as exc:  # noqa: BLE001 - provenance is best-effort, never fatal
        print(
            f"[warn] could not read the plugin registry: {exc}",
            file=sys.stderr,
        )
    return os.environ.get("PXR_PLUGINPATH_NAME", "")


def ayon_resolver_loaded(uri: str) -> bool:
    """True when the resolver resolves ``uri`` to a non-empty path different from the input.

    The only honest check: the AYON resolver is a primary ArResolver, invisible to
    Ar.GetRegisteredURISchemes() (see _registered_uri_schemes). ``uri`` must be one that
    actually exists on the target server — a made-up URI resolves to nothing (or itself)
    whether the resolver works or not.

    Returns:
        True when the URI resolves to a real, different path.
    """
    Ar, _, _ = _require_pxr()  # noqa: N806 - pxr module objects; lowercase would misname them
    resolved = str(Ar.GetResolver().Resolve(uri))
    return bool(resolved) and resolved != uri


def _assert_resolver(verify_uri: str | None, *, allow_missing: bool) -> None:
    """Refuse to measure without a working AYON resolver — every number would be a lie.

    A bare hython has no AYON resolver; it is wired by the ayon-usd addon's pre-launch
    hook. Without it every ayon:// URI resolves to itself, stage open is fast and empty,
    and the whole matrix reports plausible-looking timings for a configuration that never
    existed. The proof is resolving a known-existing URI to a different, non-empty path —
    scheme registration cannot prove anything (see _registered_uri_schemes).

    Get the env from the launcher -- never hand-assemble it. The addon owns which resolver
    build each app variant gets, and a variant with no `asset_resolvers` entry is SUPPOSED
    to launch without one, so a hand-set PXR_PLUGINPATH_NAME can fake a configuration the
    studio would never actually run.
    """
    if allow_missing:
        return
    if not verify_uri:
        sys.exit(
            "--verify-uri is required: a known-existing ayon+entity:// URI, e.g.\n"
            "  ayon+entity://PROJ/path/to/folder?product=usdAsset&version=v001&representation=usd\n"
            "Pass --allow-no-resolver only to smoke-test the harness itself."
        )
    Ar, _, _ = _require_pxr()  # noqa: N806 - pxr module objects; lowercase would misname them
    resolved = str(Ar.GetResolver().Resolve(verify_uri))
    if resolved and resolved != verify_uri:
        return
    sys.exit(
        f"resolver did not resolve {verify_uri} (got: {resolved or '<empty>'}).\n"
        "Registered URI schemes (diagnostic only — the AYON resolver is a primary ArResolver "
        f"and never registers a scheme): {_registered_uri_schemes()}\n"
        "Run under a launcher-built env:\n"
        "  ayon --headless addon applications extractenvironments env.json \\\n"
        "      --project P --folder /path --task T --app houdini/21-0-700-core\n"
        "  python3 ayon_env_run.py env.json <hython> resolve_bench.py ...\n"
        "If the resolver plugin IS loaded (see resolver_plugin in the probe JSON), check "
        "AYON_SERVER_URL / the API key / that the verify URI exists on that server. If it is "
        "absent, the app variant has no asset_resolvers entry, or the entry points at a path "
        "that does not exist on this host -- check before benchmarking.\n"
        "Pass --allow-no-resolver only to smoke-test the harness itself."
    )


def collect_uris(root: str, limit: int = 20000) -> list[str]:
    """BFS the composition graph for AYON URIs, approximating the prewarm pass's walk.

    Dependencies are anchored against their parent layer (CreateIdentifier), matching
    prewarm.cpp. Resolving raw dependency strings instead would send relative sublayers
    ('./x.usd') through CWD, drop them, and silently omit their AYON subtrees — making the
    resolve probe measure a smaller workload than the stage probe actually walks.

    Returns:
        Every AYON URI reachable from the root, in discovery order.
    """
    Ar, Sdf, _ = _require_pxr()  # noqa: N806 - pxr module objects; lowercase would misname them
    resolver = Ar.GetResolver()
    seen: set[str] = set()
    ordered: list[str] = []
    frontier = [root]

    while frontier and len(ordered) < limit:
        nxt: list[str] = []
        for identifier in frontier:
            clean = identifier.split(":SDF_FORMAT_ARGS", 1)[0]
            if clean in seen:
                continue
            seen.add(clean)
            if clean.startswith(("ayon://", "ayon+entity://")):
                ordered.append(clean)
            try:
                layer = Sdf.Layer.FindOrOpen(
                    str(resolver.Resolve(identifier)) or identifier
                )
            except Exception as exc:  # noqa: BLE001 - a bad ref must not kill the walk
                print(f"[warn] open {clean}: {exc}", file=sys.stderr)
                continue
            if not layer:
                continue
            nxt.extend(
                resolver.CreateIdentifier(dep, layer.resolvedPath)
                for dep in layer.GetCompositionAssetDependencies()
            )
        frontier = nxt
    return ordered


def probe_stage(root: str) -> dict[str, Any]:
    """Macro measurement: open the stage and traverse it. The number that matters.

    Returns:
        One result row: elapsed milliseconds and the composed prim count.
    """
    _, _, Usd = _require_pxr()  # noqa: N806 - pxr module objects; lowercase would misname them
    start = time.perf_counter_ns()
    stage = Usd.Stage.Open(root)
    opened = time.perf_counter_ns()
    prims = sum(1 for _ in stage.Traverse()) if stage else 0
    done = time.perf_counter_ns()
    return {
        "probe": "stage",
        "open_ms": (opened - start) / 1e6,
        "traverse_ms": (done - opened) / 1e6,
        "total_ms": (done - start) / 1e6,
        "prims": prims,
        # prims==0 means URIs silently failed to resolve — the timing of an empty stage is
        # meaningless, and the matrix aborts on it rather than recording a fast fake.
        "composed": prims > 0,
    }


def probe_resolve(uris: list[str], repeats: int) -> dict[str, Any]:
    """Micro measurement: per-URI Ar.Resolve, first touch vs repeat.

    Only meaningful in a process that has NOT opened the stage: the first pass must find
    PreCache empty so it exercises the configured backing path. Later passes hit PreCache
    and therefore measure the resolver's own overhead, which isolates machinery cost.

    NB prewarm only triggers from _CreateDefaultContextForAsset (i.e. stage open), so the
    prewarm gate does not affect this probe — compare stage timings for that.

    Returns:
        One result row: first-touch and repeat resolve timings.
    """
    Ar, _, _ = _require_pxr()  # noqa: N806 - pxr module objects; lowercase would misname them
    resolver = Ar.GetResolver()

    def timed_pass() -> list[float]:
        out: list[float] = []
        for uri in uris:
            start = time.perf_counter_ns()
            resolver.Resolve(uri)
            out.append((time.perf_counter_ns() - start) / 1e6)
        return out

    cold = timed_pass()
    warm: list[float] = []
    for _ in range(max(0, repeats - 1)):
        warm = timed_pass()

    def summarise(samples: list[float], prefix: str) -> dict[str, Any]:
        if not samples:
            return {}
        ordered = sorted(samples)
        return {
            f"{prefix}_total_ms": sum(samples),
            f"{prefix}_mean_ms": statistics.fmean(samples),
            f"{prefix}_median_ms": statistics.median(samples),
            f"{prefix}_p95_ms": ordered[
                min(len(ordered) - 1, int(len(ordered) * 0.95))
            ],
            f"{prefix}_max_ms": ordered[-1],
        }

    return {
        "probe": "resolve",
        "count": len(uris),
        **summarise(cold, "cold"),
        **summarise(warm, "warm"),
    }


def probe_multi(roots: list[str]) -> dict[str, Any]:
    """Session measurement: open every shot stage in ONE process, in order.

    Artists open many shot USDs per Houdini/Maya session, so later shots hit the shared
    in-process cache (and memcached) for assets earlier shots already resolved — the case
    prewarm x memcached is supposed to win. All stages stay open for the whole run, like a
    real session; dropping one would release its layers and re-compose them for the next.

    Returns:
        One result row covering every shot opened in this process.
    """
    _, _, Usd = _require_pxr()  # noqa: N806 - pxr module objects; lowercase would misname them
    stages = []
    open_ms: list[float] = []
    prims: list[int] = []
    start = time.perf_counter_ns()
    for root in roots:
        opened = time.perf_counter_ns()
        stage = Usd.Stage.Open(root)
        open_ms.append((time.perf_counter_ns() - opened) / 1e6)
        prims.append(sum(1 for _ in stage.Traverse()) if stage else 0)
        stages.append(stage)
    total = (time.perf_counter_ns() - start) / 1e6
    return {
        "probe": "multi",
        "stages": len(roots),
        "open_ms_per_stage": open_ms,
        "prims_per_stage": prims,
        "total_ms": total,
        "prims": sum(prims),
        # ONE empty stage means that shot's URIs silently failed — same rule as probe_stage.
        "composed": bool(prims) and all(count > 0 for count in prims),
    }


# --- matrix side ---------------------------------------------------------------

# (name, env overrides, flush memcached first, run twice and keep the 2nd)
# Every cell sets BOTH gates explicitly. Omitting one would let an exported
# AYON_RESOLVER_NO_PREWARM in the parent env silently flip a prewarm-on cell off.
CELLS: list[tuple[str, dict[str, str], bool, bool]] = [
    (
        "0-baseline",
        {"AYON_RESOLVER_NO_PREWARM": "1", "AYON_MEMCACHED_ENABLED": "false"},
        False,
        False,
    ),
    (
        "1-prewarm-only",
        {"AYON_RESOLVER_NO_PREWARM": "0", "AYON_MEMCACHED_ENABLED": "false"},
        False,
        False,
    ),
    (
        "2-memcache-cold",
        {"AYON_RESOLVER_NO_PREWARM": "1", "AYON_MEMCACHED_ENABLED": "true"},
        True,
        False,
    ),
    (
        "3-memcache-warm",
        {"AYON_RESOLVER_NO_PREWARM": "1", "AYON_MEMCACHED_ENABLED": "true"},
        True,
        True,
    ),
    (
        "4-both-cold",
        {"AYON_RESOLVER_NO_PREWARM": "0", "AYON_MEMCACHED_ENABLED": "true"},
        True,
        False,
    ),
    (
        "5-both-warm",
        {"AYON_RESOLVER_NO_PREWARM": "0", "AYON_MEMCACHED_ENABLED": "true"},
        True,
        True,
    ),
    # memcached configured but unreachable: the resolver's isConnected() does no network I/O
    # (memcached_server_push parses, never connects), so the handler reports connected and every
    # lookup blocks for the full timeout. Servers come from --degraded-server, never --memcached,
    # and the cell runs even without --memcached — it needs no real server.
    (
        "6-degraded",
        {"AYON_RESOLVER_NO_PREWARM": "0", "AYON_MEMCACHED_ENABLED": "true"},
        False,
        False,
    ),
    # Same unreachable server, prewarm OFF. Cell 6 understates the cost of a dead cache
    # because batchWarm resolves most of the closure without ever asking memcached; with
    # prewarm off every URI goes through getAsset(), so every URI eats the full timeout.
    (
        "6b-degraded-noprewarm",
        {"AYON_RESOLVER_NO_PREWARM": "1", "AYON_MEMCACHED_ENABLED": "true"},
        False,
        False,
    ),
    # Pinning cells (need --pin-file). 7 is the pinned number; 8 and 9 exist to PROVE the
    # exclusivity claim rather than read it off the source -- 8 should match 7 (batchWarm
    # no-ops in static mode) and 9 must show a memcached stat delta of ZERO.
    (
        "7-pinning",
        {
            "AYON_RESOLVER_NO_PREWARM": "1",
            "AYON_MEMCACHED_ENABLED": "false",
            PINNING_GATE: "true",
        },
        False,
        False,
    ),
    (
        "8-pinning+prewarm",
        {
            "AYON_RESOLVER_NO_PREWARM": "0",
            "AYON_MEMCACHED_ENABLED": "false",
            PINNING_GATE: "true",
        },
        False,
        False,
    ),
    (
        "9-pinning+memcache",
        {
            "AYON_RESOLVER_NO_PREWARM": "0",
            "AYON_MEMCACHED_ENABLED": "true",
            PINNING_GATE: "true",
        },
        False,
        False,
    ),
]


def _stat_delta(
    before: dict[str, str], after: dict[str, str], key: str
) -> int:
    """Counter movement for one memcached stat across a probe run.

    Returns:
        The counter's movement across the run.
    """
    return int(after.get(key, 0)) - int(before.get(key, 0))


def run_cell(  # noqa: C901 - a flat sequence of guards; splitting it hides the order they run in
    args: argparse.Namespace, env_overrides: dict[str, str], kind: str
) -> dict[str, Any]:
    """Spawn one probe subprocess for one cell and one probe kind; parse its JSON.

    Returns:
        The probe's parsed JSON result, or a dict carrying `error`.
    """
    env = dict(os.environ)
    # The legacy gate would override the explicit one we are about to set.
    env.pop("AYON_RESOLVER_PREWARM", None)
    # Same reasoning as the two gates above, but worse if it leaks: a stray pinning export
    # turns every cell into an offline pin-file lookup that still composes, so the matrix
    # would report cache timings for a configuration that never consulted the cache.
    env[PINNING_GATE] = "false"
    if args.memcached:
        env["AYON_MEMCACHED_SERVERS"] = args.memcached
    if args.timeout_ms:
        env["AYON_MEMCACHED_TIMEOUT_MS"] = str(args.timeout_ms)
    if args.ayon_proxy_url:
        # Route resolver traffic through the (latency-injected) proxy, not the direct server.
        env["AYON_SERVER_URL"] = args.ayon_proxy_url
    # Cell overrides win last, so 6-degraded's pinned AYON_MEMCACHED_SERVERS beats --memcached.
    env.update(env_overrides)

    cmd = [args.hython, os.path.abspath(__file__), "probe", "--kind", kind]
    if kind == "multi":
        # The probe re-reads the file itself; the matrix --root (stage root) must not leak in.
        cmd += ["--roots-file", args.roots_file]
    else:
        cmd += ["--root", args.root]
    if args.verify_uri:
        cmd += ["--verify-uri", args.verify_uri]
    if args.allow_no_resolver:
        cmd.append("--allow-no-resolver")
    if kind == "resolve":
        cmd += ["--uris", args.uris, "--repeats", str(args.repeats)]

    proc = subprocess.run(
        cmd, env=env, capture_output=True, text=True, check=False
    )
    if proc.returncode != 0:
        return {"error": (proc.stderr or "probe failed").strip()[:500]}
    for line in reversed(proc.stdout.splitlines()):
        if not line.startswith("{"):
            continue
        try:
            return json.loads(line)
        except json.JSONDecodeError:
            continue  # resolver debug spew can emit brace-leading lines
    return {"error": "probe emitted no JSON"}


def _measure(  # noqa: PLR0913 - one CSV row needs every axis that identifies it
    args: argparse.Namespace,
    name: str,
    overrides: dict[str, str],
    kind: str,
    rtt: int | None,
    *,
    degraded: bool = False,
) -> dict[str, Any]:
    """One measured cell: stats snapshot around the probe. The caller enforces `composed`.

    Returns:
        The finished CSV row for this cell.
    """
    stat_server = "" if degraded else (args.memcached or "")
    before = memcached_stats(stat_server) if stat_server else {}
    print(
        f"[run ] {name} ({kind}) rtt={rtt if rtt is not None else '-'}",
        file=sys.stderr,
    )
    result = run_cell(args, overrides, kind)
    after = memcached_stats(stat_server) if stat_server else {}

    if degraded:
        # The blackhole endpoint is never flushed or statted; NA, not 0, so nobody reads
        # "0 hits" as "the cache was consulted and missed".
        mc_cols: dict[str, Any] = {
            "mc_get_hits": "",
            "mc_get_misses": "",
            "mc_cmd_set": "",
            "mc_curr_items": "",
        }
    else:
        mc_cols = {
            "mc_get_hits": _stat_delta(before, after, "get_hits"),
            "mc_get_misses": _stat_delta(before, after, "get_misses"),
            "mc_cmd_set": _stat_delta(before, after, "cmd_set"),
            "mc_curr_items": after.get("curr_items", ""),
        }
    return {
        "cell": name,
        "kind": kind,
        "rtt_ms": "" if rtt is None else rtt,
        **result,
        **mc_cols,
    }


def _write_csv(rows: list[dict[str, Any]], out: str) -> None:
    """Write rows with the union of all columns (probe kinds report different fields)."""
    fields: list[str] = []
    for row in rows:
        fields.extend(k for k in row if k not in fields)
    with pathlib.Path(out).open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def _run_level(  # noqa: C901 - one branch per cell kind; flattening is what keeps them comparable
    args: argparse.Namespace,
    kinds: list[str],
    rtt: int | None,
    rows: list[dict[str, Any]],
    abort: Callable[[str], NoReturn],
) -> None:
    """Run the full cell matrix once (at one RTT level), appending finished rows."""
    for name, cell_env, flush, warm in CELLS:
        if args.only and not any(pat in name for pat in args.only):
            continue
        overrides = dict(cell_env)
        degraded = "degraded" in name
        if degraded:
            overrides["AYON_MEMCACHED_SERVERS"] = args.degraded_server
            print(
                f"[warn] {name}: memcached deliberately unreachable ({args.degraded_server}) — every lookup "
                f"blocks for AYON_MEMCACHED_TIMEOUT_MS ({args.timeout_ms or 1000}ms); at ~200 URIs x 1s "
                "this cell takes MINUTES by design",
                file=sys.stderr,
            )
        if overrides.get(PINNING_GATE) == "true":
            if not args.pin_file:
                print(f"[skip] {name}: --pin-file not set", file=sys.stderr)
                continue
            overrides[PINNING_FILE] = args.pin_file
        needs_memcached = overrides.get("AYON_MEMCACHED_ENABLED") == "true"
        if needs_memcached and not degraded and not args.memcached:
            print(f"[skip] {name}: --memcached not set", file=sys.stderr)
            continue
        for kind in kinds:
            if (
                flush
                and args.memcached
                and not memcached_flush(args.memcached)
            ):
                print(
                    f"[skip] {name}: memcached {args.memcached} unreachable",
                    file=sys.stderr,
                )
                continue
            if warm:
                # Populate the cache, then measure the second run against it. A failed
                # warm-up would leave the cache cold under a "warm" label.
                seed_before = (
                    memcached_stats(args.memcached) if args.memcached else {}
                )
                seed = run_cell(args, overrides, kind)
                seed_after = (
                    memcached_stats(args.memcached) if args.memcached else {}
                )
                if seed.get("error"):
                    abort(
                        f"{name}: warm-up run failed, cannot measure a warm cache: {seed['error']}"
                    )
                if (
                    args.memcached
                    and _stat_delta(seed_before, seed_after, "cmd_set") <= 0
                ):
                    abort(
                        f"{name}: warm-up wrote nothing to memcached — the cache is not warm."
                    )
            row = _measure(args, name, overrides, kind, rtt, degraded=degraded)
            if row.get("composed") is False:
                abort(
                    f"{name} ({kind}): a probed stage composed 0 prims — its ayon:// URIs failed to "
                    "resolve. The resolver is loaded but not functional (check AYON_SERVER_URL / API "
                    "key / that the closure exists on disk). Refusing to record fake timings."
                )
            rows.append(row)


def cmd_matrix(args: argparse.Namespace) -> int:  # noqa: C901, PLR0912, PLR0915 - argument validation, one guard per way a run goes wrong
    """Run every configuration (per RTT level) in its own process and write the CSV.

    Returns:
        Process exit status.
    """
    if not args.verify_uri and not args.allow_no_resolver:
        sys.exit(
            "--verify-uri is required: a known-existing ayon+entity:// URI that every probe "
            "must resolve before measuring. Pass --allow-no-resolver only to smoke-test the "
            "harness itself."
        )
    if args.rtt_ms and not args.ayon_proxy_url:
        sys.exit(
            "--rtt-ms without --ayon-proxy-url: the latency toxic would apply to a proxy no probe "
            "uses — every row would claim an RTT the resolver never saw. Pass --ayon-proxy-url "
            "(e.g. http://toxiproxy:15000) so probe traffic actually routes through the proxy."
        )
    if args.rtt_ms and any(level < 0 for level in args.rtt_ms):
        sys.exit("--rtt-ms must be >= 0")
    multi_roots: list[str] = []
    if args.roots_file:
        multi_roots = load_multi_roots(args.roots_file)
        if not multi_roots:
            sys.exit(
                f"{args.roots_file} yields no roots — nothing for the multi probe to open"
            )
    kinds = (
        ["stage"]
        + (["resolve"] if args.uris else [])
        + (["multi"] if multi_roots else [])
    )
    rows: list[dict[str, Any]] = []

    def abort(message: str) -> NoReturn:
        """Fail loudly, but write the evidence collected so far first."""
        if rows:
            _write_csv(rows, args.out)
            message += f"\nPARTIAL results ({len(rows)} rows, completed cells only) written to {args.out}."
        else:
            message += "\nNo cells had completed; nothing written."
        sys.exit(message)

    levels: list[int | None] = list(args.rtt_ms) if args.rtt_ms else [None]
    cleanup_failed = False
    for level in levels:
        if level is not None:
            # A crashed earlier run may have left its toxic behind — never measure through it.
            try:
                toxiproxy_delete_toxic(
                    args.toxiproxy_api, args.toxiproxy_proxy
                )
            except (urllib.error.URLError, OSError) as exc:
                abort(f"toxiproxy API {args.toxiproxy_api} unreachable: {exc}")
        try:
            if level:
                try:
                    toxiproxy_create_toxic(
                        args.toxiproxy_api, args.toxiproxy_proxy, level
                    )
                except (urllib.error.URLError, OSError) as exc:
                    abort(
                        f"could not install the {level}ms toxic on proxy '{args.toxiproxy_proxy}': {exc}"
                    )
                try:
                    measured = measure_proxy_rtt(
                        args.ayon_proxy_url, timeout_s=10 + level * 4 / 1000
                    )
                except (urllib.error.URLError, OSError) as exc:
                    abort(
                        f"rtt={level}ms: cannot verify the toxic — request through {args.ayon_proxy_url} "
                        f"failed: {exc}. Refusing to record rows labelled with an unverified latency."
                    )
                if measured < 0.8 * level:
                    abort(
                        f"rtt={level}ms requested but a request through {args.ayon_proxy_url} took "
                        f"{measured:.1f}ms (< 80% of requested) — the toxic did not take effect. "
                        f"Refusing to record rows labelled rtt_ms={level}."
                    )
                print(
                    f"[rtt ] {level}ms toxic verified: /api/info via proxy took {measured:.1f}ms",
                    file=sys.stderr,
                )
            _run_level(args, kinds, level, rows, abort)
        finally:
            # Runs on abort too (SystemExit passes through): a leftover toxic poisons later runs.
            if level:
                try:
                    toxiproxy_delete_toxic(
                        args.toxiproxy_api, args.toxiproxy_proxy
                    )
                except (urllib.error.URLError, OSError) as exc:
                    cleanup_failed = True
                    print(
                        f"[FATAL] rtt toxic may still be set on '{args.toxiproxy_proxy}' ({exc}); remove it: "
                        f"curl -X DELETE {args.toxiproxy_api}/proxies/{args.toxiproxy_proxy}/toxics/{RTT_TOXIC}",
                        file=sys.stderr,
                    )

    if not rows:
        print("no cells ran", file=sys.stderr)
        return 1

    _write_csv(rows, args.out)
    for row in rows:
        summary = (
            row.get("error")
            or f"{row.get('total_ms', row.get('cold_total_ms', '?'))} ms"
        )
        rtt_label = str(row["rtt_ms"]) or "-"
        print(
            f"{row['cell']:<18} {row['kind']:<8} rtt={rtt_label:<5} {summary}   "
            f"hits={row['mc_get_hits']} misses={row['mc_get_misses']}"
        )
    print(f"\nwrote {args.out}", file=sys.stderr)
    return 1 if cleanup_failed else 0


def cmd_probe(args: argparse.Namespace) -> int:
    """Measure the configuration this process was started with; emit one JSON line.

    Returns:
        Process exit status.
    """
    roots = args.root or []
    multi_roots: list[str] = []
    if args.kind == "multi":
        multi_roots = (
            load_multi_roots(args.roots_file) if args.roots_file else []
        ) + roots
        if not multi_roots:
            sys.exit(
                "probe --kind multi needs roots: --roots-file and/or repeated --root"
            )
    elif len(roots) != 1:
        sys.exit(f"probe --kind {args.kind} needs exactly one --root")
    # NB: verification resolves verify_uri before measurement, seeding PreCache (and possibly
    # memcached) with that ONE entry — noise against a real closure, but pick a verify URI
    # outside --uris/--root if single-URI purity matters.
    _assert_resolver(args.verify_uri, allow_missing=args.allow_no_resolver)
    result: dict[str, Any] = {
        "prewarm": prewarm_enabled_from_env(),
        "memcached": os.environ.get("AYON_MEMCACHED_ENABLED") == "true",
        "servers": os.environ.get("AYON_MEMCACHED_SERVERS", ""),
        "ayon_resolver": bool(args.verify_uri)
        and ayon_resolver_loaded(args.verify_uri),
        "uri_schemes": _registered_uri_schemes(),  # diagnostic only — see _registered_uri_schemes
        "resolver_plugin": resolver_plugin_path(),
    }
    if args.kind == "stage":
        result.update(probe_stage(roots[0]))
    elif args.kind == "multi":
        result.update(probe_multi(multi_roots))
    else:
        with pathlib.Path(args.uris).open(encoding="utf-8") as handle:
            uris = [line.strip() for line in handle if line.strip()]
        result.update(probe_resolve(uris, args.repeats))
    print(json.dumps(result))
    return 0


def cmd_collect(args: argparse.Namespace) -> int:
    """Write the stage's AYON URI closure to a file for the resolve probe.

    Returns:
        Process exit status.
    """
    # Without the resolver every ayon:// ref fails to open, so the BFS stops at the root
    # and silently writes a 1-line closure.
    _assert_resolver(args.verify_uri, allow_missing=args.allow_no_resolver)
    uris = collect_uris(args.root)
    pathlib.Path(args.out).write_text("\n".join(uris) + "\n", encoding="utf-8")
    print(f"{len(uris)} URIs -> {args.out}", file=sys.stderr)
    return 0


def main() -> int:
    """Parse arguments and dispatch to the matrix / probe / collect commands.

    Returns:
        Process exit status.
    """
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    matrix = sub.add_parser(
        "matrix", help="run every configuration, write CSV"
    )
    matrix.add_argument(
        "--hython", required=True, help="DCC interpreter that runs the probes"
    )
    matrix.add_argument("--root", required=True, help="stage root URI")
    matrix.add_argument(
        "--uris", help="URI list file (from `collect`); adds the resolve probe"
    )
    matrix.add_argument(
        "--roots-file",
        help="JSON root list ({'level','uri'|'uris'} objects; 'uris' entries are the multi-shot "
        "root set); adds the multi probe",
    )
    matrix.add_argument("--repeats", type=int, default=3)
    matrix.add_argument(
        "--rtt-ms",
        type=int,
        action="append",
        help="RTT level to sweep via toxiproxy (repeatable; 0 = through the proxy, no toxic). "
        "Requires --ayon-proxy-url; each level is verified by measurement before any row is recorded",
    )
    matrix.add_argument(
        "--ayon-proxy-url",
        help="AYON_SERVER_URL handed to every probe, pointing at the toxiproxy listener (e.g. http://toxiproxy:15000)",
    )
    matrix.add_argument(
        "--toxiproxy-api",
        default="http://toxiproxy:8474",
        help="toxiproxy admin API URL",
    )
    matrix.add_argument(
        "--toxiproxy-proxy",
        default="ayon",
        help="toxiproxy proxy name fronting the AYON server",
    )
    matrix.add_argument(
        "--memcached",
        help="host:port used for flush + stats (DEDICATED instance only)",
    )
    matrix.add_argument(
        "--degraded-server",
        default=DEGRADED_MEMCACHED_SERVER,
        help="unreachable host:port for the 6-degraded cell (must blackhole, not refuse — see the constant)",
    )
    matrix.add_argument(
        "--timeout-ms", type=int, help="AYON_MEMCACHED_TIMEOUT_MS override"
    )
    matrix.add_argument(
        "--pin-file",
        help="pin file (as the PROBE sees it) enabling the 7/8/9-pinning cells; --verify-uri must be "
        "one of its keys, since static mode has no fall-through to resolve anything else",
    )
    matrix.add_argument(
        "--verify-uri",
        help="known-existing ayon+entity:// URI every probe must resolve before measuring "
        "(REQUIRED unless --allow-no-resolver; forwarded to every probe subprocess)",
    )
    matrix.add_argument(
        "--allow-no-resolver",
        action="store_true",
        help="skip resolver verification in every probe (harness smoke-test only)",
    )
    matrix.add_argument(
        "--only",
        action="append",
        help="run only cells whose name contains this substring (repeatable) -- e.g. --only pinning to add "
        "an axis to an existing CSV without re-measuring the cells you already have",
    )
    matrix.add_argument("--out", default="results.csv")
    matrix.set_defaults(func=cmd_matrix)

    probe = sub.add_parser(
        "probe", help="measure ONE configuration (run inside the DCC)"
    )
    probe.add_argument(
        "--root",
        action="append",
        help="stage root URI (repeatable for --kind multi)",
    )
    probe.add_argument(
        "--kind", choices=("stage", "resolve", "multi"), default="stage"
    )
    probe.add_argument("--uris")
    probe.add_argument(
        "--roots-file",
        help="JSON root list ({'level','uri'|'uris'} objects; 'uris' entries are the multi-shot "
        "root set) for --kind multi",
    )
    probe.add_argument("--repeats", type=int, default=3)
    probe.add_argument(
        "--verify-uri",
        help="known-existing URI the resolver must resolve before measuring (REQUIRED unless --allow-no-resolver)",
    )
    probe.add_argument(
        "--allow-no-resolver",
        action="store_true",
        help="measure even without a working AYON resolver (harness smoke-test only)",
    )
    probe.set_defaults(func=cmd_probe)

    collect = sub.add_parser("collect", help="BFS a stage's AYON URI closure")
    collect.add_argument("--root", required=True)
    collect.add_argument("--out", default="uris.txt")
    collect.add_argument(
        "--verify-uri",
        help="known-existing URI the resolver must resolve before collecting (REQUIRED unless --allow-no-resolver)",
    )
    collect.add_argument(
        "--allow-no-resolver",
        action="store_true",
        help="skip the AYON resolver check",
    )
    collect.set_defaults(func=cmd_collect)

    args = parser.parse_args()
    return int(args.func(args))


if __name__ == "__main__":
    sys.exit(main())
