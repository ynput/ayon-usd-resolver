"""Stage open time against memcached round-trip time, for comparing builds in Grafana.

Asserts composition only. Timing comes from the session records (``open_ms``, ``rtt_ms``,
``mc_get_commands``), with one series per ``build`` label. The numbers depend on the machine,
so a threshold here would only make the test flaky.
"""

from __future__ import annotations

import statistics

import pytest

from lib.mc import memcached_knobs

REPEATS = 5


@pytest.mark.parametrize("rtt_ms", [0, 2, 5])
def test_open_time_by_round_trip(project, backend, run_resolver, mc_proxy, rtt_ms: int) -> None:
    client = backend.client()
    knobs = memcached_knobs(backend)
    proxy = mc_proxy(rtt_ms=rtt_ms)
    summary: dict[str, dict[str, float]] = {}

    for session in ("cold", "warm"):
        times, commands, ayon = [], [], []
        for _ in range(REPEATS):
            if session == "cold":
                client.flush()
            result = run_resolver(session, proxy=proxy, **knobs)
            assert result.ok, result.describe()
            assert result.resolved == project.expected, result.describe()
            times.append(result.data["open_seconds"] * 1000)
            commands.append(result.mc["mc_get_commands"])
            ayon.append(result.tap["resolve_uris_total"])
        summary[session] = {
            "open_ms_median": round(statistics.median(times), 1),
            "get_commands": statistics.median(commands),
            "ayon_uris": statistics.median(ayon),
        }
    print(f"\nLATENCY backend={backend.name} rtt_ms={rtt_ms} {summary}")
