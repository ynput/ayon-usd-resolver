"""Client for the ayon-tap control API (see tap/ayon_tap.py)."""

from __future__ import annotations

import json
import urllib.request
from typing import Any


class TapClient:
    def __init__(self, base_url: str) -> None:
        self.base_url = base_url.rstrip("/")

    def _call(self, method: str, action: str, payload: dict[str, Any] | None = None) -> dict[str, Any]:
        data = json.dumps(payload).encode() if payload is not None else None
        request = urllib.request.Request(
            f"{self.base_url}/{action}",
            data=data,
            method=method,
            headers={"Content-Type": "application/json"} if data else {},
        )
        with urllib.request.urlopen(request, timeout=10) as response:
            return json.load(response)

    def counts(self) -> dict[str, Any]:
        return self._call("GET", "counts")

    def reset(self, traceparent: str | None = None) -> None:
        """Zero the counters; spans for later requests are parented under ``traceparent``."""
        self._call("POST", "reset", {"traceparent": traceparent} if traceparent else None)

    def add_fault(
        self,
        path_regex: str,
        *,
        status: int | None = None,
        delay_ms: int = 0,
        method: str | None = None,
        body: Any = None,  # ruff: ignore[any-type] - any JSON value
    ) -> None:
        """Delay matching requests, then answer with ``status`` and, if given, ``body`` as JSON."""
        self._call(
            "POST",
            "fault",
            {"path_regex": path_regex, "status": status, "delay_ms": delay_ms, "method": method, "body": body},
        )

    def clear_faults(self) -> None:
        self._call("DELETE", "fault")
