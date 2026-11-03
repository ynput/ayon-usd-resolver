"""Upload resolver-it.json to Grafana through its HTTP API.

    GRAFANA_URL=http://localhost:3000 GRAFANA_TOKEN=glsa_... python publish_dashboard.py

``GRAFANA_TOKEN`` is a service-account token with the Editor role. Basic auth works too
(``GRAFANA_USER`` / ``GRAFANA_PASSWORD``). The dashboard goes into the folder named by
``GRAFANA_FOLDER`` (default "AYON"), created if missing, and overwrites an earlier upload
with the same uid.

Alternative without credentials: copy resolver-it.json into a folder under ayon-vector's
``services/grafana-provisioning/dashboards/json/``; Grafana picks it up within 30 s.
"""

from __future__ import annotations

import base64
import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any


def _headers() -> dict[str, str]:
    headers = {"Content-Type": "application/json"}
    if token := os.environ.get("GRAFANA_TOKEN"):
        headers["Authorization"] = f"Bearer {token}"
    elif user := os.environ.get("GRAFANA_USER"):
        basic = base64.b64encode(f"{user}:{os.environ.get('GRAFANA_PASSWORD', '')}".encode()).decode()
        headers["Authorization"] = f"Basic {basic}"
    else:
        sys.exit("set GRAFANA_TOKEN (or GRAFANA_USER and GRAFANA_PASSWORD)")
    return headers


def _call(base: str, method: str, path: str, payload: dict[str, Any] | None = None) -> Any:  # ruff: ignore[any-type] - Grafana API JSON
    request = urllib.request.Request(
        base + path,
        data=json.dumps(payload).encode() if payload is not None else None,
        headers=_headers(),
        method=method,
    )
    with urllib.request.urlopen(request, timeout=15) as response:
        return json.load(response)


def _folder_uid(base: str, title: str) -> str:
    for folder in _call(base, "GET", "/api/folders"):
        if folder["title"] == title:
            return folder["uid"]
    return _call(base, "POST", "/api/folders", {"title": title})["uid"]


def main() -> int:
    base = os.environ.get("GRAFANA_URL", "http://localhost:3000").rstrip("/")
    dashboard = json.loads(Path(__file__).with_name("resolver-it.json").read_text(encoding="utf-8"))
    payload = {
        "dashboard": {**dashboard, "id": None},
        "folderUid": _folder_uid(base, os.environ.get("GRAFANA_FOLDER", "AYON")),
        "overwrite": True,
        "message": "ayon-usd-resolver tests/integration/grafana/publish_dashboard.py",
    }
    try:
        result = _call(base, "POST", "/api/dashboards/db", payload)
    except urllib.error.HTTPError as exc:
        print(f"upload failed: {exc.code} {exc.read().decode(errors='replace')}", file=sys.stderr)
        return 1
    print(f"{base}{result['url']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
