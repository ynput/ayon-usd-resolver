"""Run resolver scenarios in fresh processes with an explicit environment."""

from __future__ import annotations

import json
import os
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

PROBE = Path(__file__).with_name("probe.py")

# Every variable that changes resolver behavior. Each run starts from the runner's
# environment with all of these removed, then applies only what the scenario sets, so a
# value exported in the container can never leak into a scenario that didn't ask for it.
RESOLVER_KNOBS = (
    "AYON_MEMCACHED_ENABLED",
    "AYON_MEMCACHED_SERVERS",
    "AYON_MEMCACHED_TIMEOUT_MS",
    "AYON_MEMCACHED_EXPIRE_SECONDS",
    "AYON_MEMCACHED_HERO_EXPIRE_SECONDS",
    "AYON_MEMCACHED_LATEST_EXPIRE_SECONDS",
    "AYON_MEMCACHED_MISS_EXPIRE_SECONDS",
    "AYON_RESOLVER_NO_PREWARM",
    "AYON_RESOLVER_PREWARM",
    "AYON_USD_RESOLVER_ENABLE_PINNING",
    "AYON_USD_RESOLVER_PINNING_FILE",
    "AYON_USD_RESOLVER_PINNING_ROOTS",
    "TF_DEBUG",
)


@dataclass
class ProbeResult:
    ok: bool
    data: dict[str, Any]
    log: str
    env: dict[str, str] = field(repr=False)
    # ayon-tap counters for this session alone; filled in by the run_resolver fixture
    tap: dict[str, Any] = field(default_factory=dict)
    # memcached counter changes across this session; filled in by the run_resolver fixture
    mc: dict[str, int] = field(default_factory=dict)

    @property
    def resolved(self) -> dict[str, str]:
        return self.data.get("resolved", {})

    @property
    def steps(self) -> list[dict[str, Any]]:
        return self.data.get("steps", [])

    @property
    def prims(self) -> int:
        return self.data.get("prims", 0)

    def describe(self) -> str:
        """Failure context for assertion messages: the error and the log tail."""
        tail = "\n".join(self.log.splitlines()[-60:])
        knobs = {k: v for k, v in self.env.items() if k in RESOLVER_KNOBS}
        tap = {k: self.tap.get(k) for k in ("requests", "error_count", "errors")}
        return f"knobs={knobs}\nayon={tap}\nerror={self.data.get('error')}\n--- log tail ---\n{tail}"


def run_probe(
    *,
    root: str | None,
    uris: list[str],
    steps: list[dict[str, Any]] | None = None,
    base_env: dict[str, str],
    knobs: dict[str, str],
    workdir: Path,
    name: str,
    timeout: float = 300,
    command: list[str] | None = None,
) -> ProbeResult:
    workdir.mkdir(parents=True, exist_ok=True)
    uris_file = workdir / f"{name}.uris.json"
    out_file = workdir / f"{name}.result.json"
    log_file = workdir / f"{name}.log"
    out_file.unlink(missing_ok=True)  # a test can run one session name twice; never read stale output
    if steps is None:
        uris_file.write_text(json.dumps(uris))
        arguments = ["--root", str(root), "--uris", str(uris_file)]
    else:
        steps_file = workdir / f"{name}.steps.json"
        steps_file.write_text(json.dumps(steps))
        arguments = ["--steps", str(steps_file)]

    env = {k: v for k, v in os.environ.items() if k not in RESOLVER_KNOBS}
    env.update(base_env)
    # None unsets a variable, including one the runner itself has.
    env.update({k: v for k, v in knobs.items() if v is not None})
    for key in (k for k, v in knobs.items() if v is None):
        env.pop(key, None)

    with log_file.open("w") as log:
        proc = subprocess.run(
            [*(command or [sys.executable, str(PROBE)]), *arguments, "--out", str(out_file)],
            env=env,
            stdout=log,
            stderr=subprocess.STDOUT,
            timeout=timeout,
            check=False,
        )
    data = json.loads(out_file.read_text()) if out_file.exists() else {"error": f"probe exited {proc.returncode}"}
    return ProbeResult(ok=proc.returncode == 0 and data.get("ok", False), data=data, log=log_file.read_text(), env=env)
