# Resolver benchmark harness

Measures the resolver's configurations against each other — prewarm, memcached, pinning — and
verifies the cache's on-the-wire contract.

Stdlib only, so `resolve_bench.py` runs unmodified in any DCC interpreter that has `pxr`.

## What this is for

Two situations, and they need different setups:

- **A multi-site studio**, where clients are far from a single AYON server and each site runs its
  own cache. Run the matrix from each site against that site's cache; the RTT axis is whatever the
  network already gives you, so `--rtt-ms` is unnecessary.
- **A single developer with a local AYON server**, simulating a faraway one. A local server answers
  in under a millisecond, so every configuration looks identical and the cache appears worthless —
  which is why the RTT axis exists. Put a toxiproxy in front of the server and sweep `--rtt-ms` to
  reproduce a remote client without needing one.

Either way the question is the same: at what round-trip time does the cache start paying, and does
it still behave correctly when it is cold, warm, or unreachable.

| file | purpose |
|---|---|
| `resolve_bench.py` | the harness — cells × probe kinds × RTT levels, writes CSV |
| `check_rootless.py` | asserts cached values are rootless (`{root[...]}/…`), not absolute |

## Prerequisites

- **Published USD content with a real `ayon://` closure.** The harness measures composition, so a
  project of flat layers tells you nothing. Aim for a shot whose closure is 100+ URIs with
  sublayers and references, plus a set of shots to open in one session.
- **A launcher-built environment**, never hand-assembled — the addon decides which resolver build
  each app variant gets, and a variant with no `asset_resolvers` entry is *supposed* to launch
  without one:
  ```bash
  ayon --headless addon applications extractenvironments env.json \
      --project P --folder /path/to/folder --task T --app houdini/21-0-700-core
  ```
- **A running toxiproxy** in front of the AYON server, for the RTT axis only. The harness installs
  and removes the latency toxic itself, but does not create the proxy. Without it the matrix still
  runs, but every cell measures against whatever latency your server already has — which, for a
  local server, is none, and a cache has nothing to save.

## Usage

```bash
# collect a stage's URI closure (approximates the prewarm BFS)
<hython> resolve_bench.py collect --root 'ayon://PROJ//path?product=X&version=1&representation=usd' \
    --verify-uri 'ayon://PROJ//path?product=X&version=1&representation=usd' --out uris.txt

# the matrix
<hython> resolve_bench.py matrix --hython <hython> \
    --root 'ayon://PROJ//…' --uris uris.txt --roots-file roots.json \
    --verify-uri 'ayon://PROJ//…' --memcached cache-host:11211 \
    --rtt-ms 0 --rtt-ms 150 --ayon-proxy-url http://toxiproxy:15000 \
    --out results.csv

# cache contract: exits non-zero if any sampled value is absolute
python3 check_rootless.py cache-host:11211 pins.json
```

`--verify-uri` must name a URI that **actually exists**. The AYON resolver is a *primary*
`ArResolver` — it registers no URI scheme and never appears in `Ar.GetRegisteredURISchemes()` — so
the only honest proof it is working is resolving a real URI to a real, different path. A fabricated
URI resolves to nothing whether the resolver works or not.

## Cells

`0-baseline` (no prewarm, no cache) · `1-prewarm-only` · `2-memcache-cold` · `3-memcache-warm` ·
`4-both-cold` · `5-both-warm` · `6-degraded` (cache configured but unreachable) ·
`6b-degraded-noprewarm` · `7/8/9-pinning` (needs `--pin-file`)

`--only <substring>` runs a subset, so a new axis can be added to an existing result set without
re-measuring what you already have.

## Why the harness is mostly guards

A benchmark that reports plausible timings for a configuration that never existed is worse than no
benchmark. Each guard exists because something nearly got past it:

- **Fresh process per cell.** Both feature gates are read once per process — prewarm caches its gate
  in a function-local static, memcached connects at cache construction — so flipping env inside a
  live process measures nothing.
- **One probe kind per process.** A stage open fills the in-process PreCache; a resolve probe after
  it times PreCache hits, not the configured backing path.
- **Latency measured, not requested.** Each RTT level installs the toxic, times a real request
  through the proxy, and aborts unless ≥80% of the requested latency materialised.
- **Abort on an empty compose.** A stage composing zero prims fails the cell. Prim counts ride in
  every row, so a truncated dependency walk is visible instead of looking like a speedup.
- **Pinning gate forced off per cell.** The resolver treats any value but the literal `false` as ON,
  so a stray `AYON_USD_RESOLVER_ENABLE_PINNING=0` would silently turn every cell into an offline
  pin-file lookup that still composes correctly.

## Reading the results

Every row carries its build, cell, RTT, prim count, resolver `.so` path and memcached counter
deltas — so a row is interpretable without the notes that produced it.

Two caveats before quoting numbers:

- **Single runs, not averages.** Repeats of the same configuration can vary appreciably, most of it
  on the sub-second rows. Treat them as order-of-magnitude, or add repeats.
- **The dead-cache penalty scales with `AYON_MEMCACHED_TIMEOUT_MS`, not closure size**, because
  libmemcached backs off a failed server rather than timing out per lookup. Any measurement of that
  cell is meaningless without its timeout.
