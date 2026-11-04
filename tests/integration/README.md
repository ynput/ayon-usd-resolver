# Resolver integration tests

These tests run the resolver against a live AYON server and real memcached servers. The resolver
is built against standalone OpenUSD in a Linux container. No DCC is needed.

Assertions use direct counters only. Nothing depends on logs or telemetry.

## Pieces

| Service | Role |
|---|---|
| `runner` | Rocky 9 with OpenUSD (`USD_VERSION`, default `v25.05`), the resolver built `--with-memcached` from this checkout, and pytest |
| `ayon-tap` | Reverse proxy in front of AYON. Counts requests per endpoint and per resolved URI, and can inject delays or error statuses (`tap/ayon_tap.py`) |
| `memcached` | Reference memcached 1.6 (text protocol, full `stats`) |
| `dragonfly` | [Dragonfly](https://www.dragonflydb.io/) with its memcached text API, a protocol-compatible alternative. Its `stats` has no hit counters, so counter checks are skipped on it |
| AYON | Not started here. By default the tests use the server from `ayon-docker`, which is reachable as `server:5000` on the `ayon-docker_default` network |

Each session creates its own throw-away project (`resolverit_<hex>`) with
`seed/seed_project.py`. It contains:

- `N` assets, each with versions 1 and 2 and a hero;
- a layout that references every asset, cycling through versions 1, 2 and hero;
- a shot that sublayers the layout.

The project is created with [pytest-ayon](https://github.com/ynput/pytest-ayon) (`resolve_anatomy`,
`create_project`, `create_representation`), using a server anatomy preset (`RIT_ANATOMY_PRESET`,
default `__primary__`). The preset's roots are replaced by a single `work` root at the runner's
`/data/work` volume. Representations use the preset's own `publish` and `hero` templates, so
file locations are decided by the anatomy, not by the tests. After creating the entities, the
seed asks `/api/resolve` for every URI and writes each layer to the path the server returns.
That is the same path the resolver will get. The session deletes the project when it finishes,
along with every file the seed wrote and any directories that leaves empty. Failed seeding cleans
up the same way.

Before seeding, the session registers `AYON_SITE_ID` with the server (`pytest_ayon.register_site`).
It does this the same way the launcher does: an authenticated `GET /api/info` with
`x-ayon-site-id` and `x-ayon-platform` headers. Without this, `/api/resolve` answers `400 Site not found` for every
URI. The site record stays on the server and is reused by later runs.

## Running

```bash
export AYON_API_KEY=...        # service user key on the AYON server
docker compose -f tests/integration/compose.yml build runner      # first build ~15-30 min (OpenUSD)
docker compose -f tests/integration/compose.yml run --rm runner   # pytest -v integration
docker compose -f tests/integration/compose.yml down
```

Extra pytest arguments replace the default command:

```bash
docker compose -f tests/integration/compose.yml run --rm runner -v -k cold integration
```

The tests directory is mounted into the runner, not copied into the image, so changing a test
needs no rebuild. Changing resolver sources does: `build runner` rebuilds only the resolver
stage, because the OpenUSD stage is cached.

| Variable | Default | Meaning |
|---|---|---|
| `AYON_API_KEY` | (required) | API key used for seeding and by the resolver |
| `AYON_DOCKER_NETWORK` | `ayon-docker_default` | Docker network the AYON server is on |
| `AYON_UPSTREAM_URL` | `http://server:5000` | AYON server address as seen from `ayon-tap` |
| `AYON_SITE_ID` | `resolver-it` | Site ID the resolver sends |
| `RIT_ASSET_COUNT` | `24` | Number of assets in the seeded closure |
| `RIT_ANATOMY_PRESET` | `__primary__` | Server anatomy preset for the seeded project (`__builtin__` also works) |
| `RIT_KEEP_PROJECT` | `0` | Set to `1` to keep the seeded project for inspection |
| `USD_VERSION` | `v25.05` | OpenUSD git tag the runner is built against |
| `DRAGONFLY_TAG` | `latest` | Dragonfly image tag |

The `backend` fixture waits up to 30 seconds for a memcached backend to answer before flushing
it, because compose only guarantees that the container was started.

## How the tests work

- **One fresh process per resolver session** (`lib/probe.py`, run by `lib/proc.py`). The
  memcached and prewarm settings are read once per process. Reusing a process would measure
  the in-process caches instead of the configuration under test.
- **No inherited resolver settings.** Every `AYON_MEMCACHED_*`, prewarm, pinning and
  `TF_DEBUG` variable is removed before a session starts. Only the scenario's own settings
  are applied.
- **AYON calls are counted by `ayon-tap`, not inferred from logs.** `result.tap` holds the
  counters for that one session.
- **Memcached contents are read directly** (`lib/mc.py`, text protocol, the same one the
  resolver uses). memc-rs is not a backend because it speaks only the binary protocol.

## Scenarios

| Test | Checks |
|---|---|
| `test_baseline_without_memcached` | The resolver composes the seeded closure using AYON alone (precondition for the other tests) |
| `test_cold_fills_cache_warm_skips_ayon` | A cold session stores every URI. A warm session in a new process composes the same stage with zero AYON resolves. Runs with and without prewarm, on each backend |
| `test_cached_values_are_rootless` | Stored values are `{root[work]}/…` and re-root to the expected path |
| `test_memcached_expiry.py` | Needs #104. TTL per URI class, clamping and invalid values, `latest` staying live, miss markers (TTL, known misses, publish after a marker, no marker on a server error, never overwriting a path), `DeleteFromCache`, and `ClearCache` without a flush |
| `test_memcached_mget.py` | Needs #105. Prewarm reads with multi-key gets, split at 256 keys, an over-long key left out of the chunk, and a dropped mget falling through to AYON |
| `test_memcached_unmapped_root.py` | Needs #106. A warm cache with a bad API key fails cleanly. A failed site-roots fetch with a working key is marked `xfail`, because #106 leaves that case open |
| `test_memcached_latency.py` | Any build. Stage open time at 0, 2 and 5 ms memcached round trip, published for comparing builds. Asserts composition only |

Until those PRs are merged, run only the files a build supports, for example
`run --rm runner -v integration/test_memcached_basic.py integration/test_memcached_latency.py`.

`lib/mcproxy.py` sits between the resolver and memcached where a test needs it. It counts `get`
commands and keys per `get`, which memcached's `stats` cannot tell apart, adds latency, and can
drop the connection on a multi-key `get`. Its counters appear in the session record as
`mc_get_commands`, `mc_get_keys`, `mc_get_max_keys` and similar, next to `rtt_ms`.

## Publishing results to Grafana (optional)

`compose.telemetry.yml` connects the runner and `ayon-tap` to the local Loki/Tempo/Grafana
stack from `ayon-vector/services` (Docker network `services_default`). Tests pass or fail the
same way with or without it. Telemetry never decides an outcome, and a failed push only logs
a warning.

```bash
export RESOLVER_REF=$(git rev-parse --short HEAD)   # build label in the records
docker compose -f tests/integration/compose.yml -f tests/integration/compose.telemetry.yml build runner
docker compose -f tests/integration/compose.yml -f tests/integration/compose.telemetry.yml run --rm runner
```

What gets published (`lib/telemetry.py`):

- **Loki**, `{job="resolver_it"}`, with labels `kind`, `run_id`, `build`, `backend` and
  `session`:
  - `kind="session"`: one JSON record per resolver process. It holds the stage open time,
    prim count, whether the result composed correctly, AYON requests/URIs/errors from
    `ayon-tap`, the memcached counter changes (`mc_*`), and `trace_id`.
  - `kind="test"`: one record per test outcome, including the failure text.
  - `kind="resolver_log"`: the resolver's own output, one line per entry.
- **Tempo**: the trace tree is `pytest.session` → `test` → `resolver.session <name>`.
  `ayon-tap` adds one span under each session for every AYON request it forwarded, with
  status and any injected fault.

Loki keeps 30 days and Tempo keeps 24 hours, so trend panels read from Loki only.

### Dashboard

`grafana/resolver-it.json` is generated by `grafana/build_dashboard.py`. Edit the script,
not the JSON. It contains:

- a run summary;
- a table of sessions, with a link to each session's trace;
- the cache effect within a run (AYON URIs, open time and memcached hits per session);
- trends across runs (warm and cold open time, AYON resolves made by warm sessions, test
  outcomes);
- failed tests, traces and resolver output.

To publish it, either:

```bash
GRAFANA_TOKEN=glsa_... python tests/integration/grafana/publish_dashboard.py   # API, folder "AYON"
```

or copy `resolver-it.json` into a folder under
`ayon-vector/services/grafana-provisioning/dashboards/json/`. Grafana provisions it within
30 seconds.
