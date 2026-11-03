"""Seed a throw-away AYON project with published USD content and a real ayon:// closure.

Layout (``N`` = asset count):

    assets/asset_00 .. asset_{N-1}   product usdAsset: versions 1, 2 and a hero (= v2)
    shots/sh010                      product usdLayout v1: references every asset
                                     product usdShot   v1: sublayers the layout  <- root URI

The layout references assets at version 1, 2 and hero in rotation, so the closure contains
all three URI classes the memcached expiry rules distinguish.

Project, anatomy and representations come from pytest-ayon: the anatomy is a server preset
(``RIT_ANATOMY_PRESET``, default ``__primary__``) with its roots replaced by one ``work``
root at the runner's ``/data/work`` volume, and representation templates are the preset's
own ``publish``/``hero`` templates. Where a file lives is therefore decided by the anatomy,
not by this script: after creating the entities it asks ``/api/resolve`` for every URI and
writes each layer to the path the server returns. That is also the path the resolver gets.

Usable as a library (``seed``/``teardown``, used by the pytest session fixture) or as a CLI
for manual runs: ``python seed_project.py seed --assets 24`` prints the manifest.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import sys
import uuid
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING, Any

from pytest_ayon import (
    create_project,
    create_session,
    delete_project,
    resolve_anatomy,
)
from pytest_ayon.utils import create_representation

if TYPE_CHECKING:
    import requests
    from pytest_ayon import ProjectInfo

ROOT_NAME = "work"
ROOT_TOKEN = f"{{root[{ROOT_NAME}]}}"
PRODUCT_TYPE = "usd"
REPRESENTATION = "usd"


@dataclass
class Manifest:
    project: str
    work_root: str
    root_uri: str
    expected_prims: int
    # uri -> absolute path the resolver must return, in closure order (root first)
    expected: dict[str, str] = field(default_factory=dict)

    @property
    def uris(self) -> list[str]:
        return list(self.expected)


def connect() -> tuple[requests.Session, str]:
    return create_session(os.environ["AYON_API_KEY"]), os.environ["AYON_SERVER_URL"].rstrip("/")


def _uri(project: str, folder_path: str, product: str, version: str) -> str:
    return f"ayon://{project}/{folder_path}?product={product}&version={version}&representation={REPRESENTATION}"


# TODO(resolver-it): drop once pytest-ayon expands `{@...}` in create_representation (develop
# 04c70b1 stores templates as written; /api/resolve then returns literal `{@version}`).
_TEMPLATE_REFERENCE = re.compile(r"\{@([A-Za-z0-9_]+)\}")


def _expand_template_references(template: str, templates: dict[str, Any]) -> str:
    """Expand anatomy ``{@name}`` references; /api/resolve formats templates as stored."""
    for _ in range(10):
        names = _TEMPLATE_REFERENCE.findall(template)
        if not names:
            return template
        for name in names:
            template = template.replace(f"{{@{name}}}", str(templates[name]))
    msg = f"anatomy template references do not resolve: {template!r}"
    raise ValueError(msg)


class _Seeder:
    def __init__(self, session: requests.Session, url: str, project: str, code: str, anatomy: dict[str, Any]) -> None:
        self.session = session
        self.url = url
        self.project = project
        self.code = code
        self.templates = anatomy["templates"]
        self.work_root = anatomy["roots"][0]["linux"]
        self.folder_types = {item["name"] for item in anatomy.get("folder_types") or []}

    def _post(self, endpoint: str, payload: dict[str, Any]) -> str:
        response = self.session.post(f"{self.url}/api/projects/{self.project}/{endpoint}", json=payload)
        if response.status_code != 201:
            msg = f"POST {endpoint} failed: {response.status_code} {response.text}"
            raise RuntimeError(msg)
        return response.json()["id"]

    def folder(self, name: str, folder_type: str, parent_id: str | None = None) -> str:
        if folder_type not in self.folder_types:
            folder_type = "Folder" if "Folder" in self.folder_types else min(self.folder_types)
        return self._post("folders", {"name": name, "folderType": folder_type, "parentId": parent_id})

    def publish(self, folder_id: str, folder_path: str, product: str, labels: list[str]) -> None:
        """Create the product, its versions ("1", "2", "hero") and a usd representation each."""
        hierarchy, _, folder_name = folder_path.rpartition("/")
        product_id = self._post("products", {"name": product, "folderId": folder_id, "productType": PRODUCT_TYPE})
        latest = max(int(v) for v in labels if v.isdigit())
        for label in labels:
            hero = label == "hero"
            number = latest if hero else int(label)
            version_id = self._post("versions", {"version": -number if hero else number, "productId": product_id})
            representation = create_representation(
                self.project,
                self.code,
                folder_name,
                "",
                product,
                number,
                version_id,
                self.templates,
                self.work_root,
                REPRESENTATION,
                template_name="hero" if hero else "publish",
                ext="usda",
                root_name=ROOT_NAME,
                product_type=PRODUCT_TYPE,
            )
            representation["data"]["context"]["hierarchy"] = hierarchy
            representation["attrib"]["template"] = _expand_template_references(
                representation["attrib"]["template"], self.templates
            )
            self._post("representations", representation)

    def resolve(self, uris: list[str]) -> dict[str, str]:
        """URI -> absolute path, as the server templates it (roots substituted here)."""
        response = self.session.post(
            f"{self.url}/api/resolve?pathOnly=true", json={"resolveRoots": False, "uris": uris}
        )
        response.raise_for_status()
        paths = {}
        for item in response.json():
            entities = item.get("entities") or []
            if not entities or not entities[-1].get("filePath"):
                msg = f"server cannot resolve seeded {item['uri']}: {item.get('error')}"
                raise RuntimeError(msg)
            rootless = entities[-1]["filePath"]
            if not rootless.startswith(ROOT_TOKEN) or "{" in rootless[len(ROOT_TOKEN) :]:
                msg = f"unexpected template result for {item['uri']}: {rootless!r}"
                raise RuntimeError(msg)
            paths[item["uri"]] = self.work_root + rootless[len(ROOT_TOKEN) :]
        return paths


def _asset_layer(name: str, label: str) -> str:
    return (
        "#usda 1.0\n"
        f'(\n    defaultPrim = "{name}"\n)\n\n'
        f'def Xform "{name}"\n{{\n'
        f'    custom string ayon:version = "{label}"\n'
        '    def Cube "geo"\n    {\n    }\n'
        "}\n"
    )


def _publish_closure(
    session: requests.Session,
    url: str,
    info: ProjectInfo,
    anatomy: dict[str, Any],
    *,
    work_root: str,
    asset_count: int,
) -> Manifest:
    """Create the closure's entities in ``info``'s project and write its layer files."""
    name = info.project_name
    seeder = _Seeder(session, url, name, info.project_code, anatomy)

    layers: dict[str, str] = {}  # uri -> layer text, for every published representation
    closure: list[str] = []

    assets_id = seeder.folder("assets", "Folder")
    rotation = ["1", "2", "hero"]
    references = []
    for i in range(asset_count):
        asset = f"asset_{i:02d}"
        folder_path = f"assets/{asset}"
        seeder.publish(seeder.folder(asset, "Asset", assets_id), folder_path, "usdAsset", rotation)
        for version in rotation:
            layers[_uri(name, folder_path, "usdAsset", version)] = _asset_layer(asset, version)
        uri = _uri(name, folder_path, "usdAsset", rotation[i % len(rotation)])
        closure.append(uri)
        references.append((asset, uri))

    shot_id = seeder.folder("sh010", "Shot", seeder.folder("shots", "Folder"))
    shot_path = "shots/sh010"
    refs = "".join(f'    def "{a}" (\n        references = @{u}@\n    )\n    {{\n    }}\n' for a, u in references)
    layout_uri = _uri(name, shot_path, "usdLayout", "1")
    layers[layout_uri] = f'#usda 1.0\n(\n    defaultPrim = "sh010"\n)\n\ndef Xform "sh010"\n{{\n{refs}}}\n'
    seeder.publish(shot_id, shot_path, "usdLayout", ["1"])
    root_uri = _uri(name, shot_path, "usdShot", "1")
    layers[root_uri] = f'#usda 1.0\n(\n    defaultPrim = "sh010"\n    subLayers = [\n        @{layout_uri}@\n    ]\n)\n'
    seeder.publish(shot_id, shot_path, "usdShot", ["1"])

    paths = seeder.resolve(list(layers))
    for uri, text in layers.items():
        target = Path(paths[uri])
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text, encoding="utf-8")

    ordered = [root_uri, layout_uri, *closure]
    # /sh010 + per asset: the referenced Xform and its Cube
    return Manifest(name, work_root, root_uri, 1 + 2 * asset_count, {uri: paths[uri] for uri in ordered})


def seed(work_root: str, asset_count: int, project: str | None = None, preset: str | None = None) -> Manifest:
    session, url = connect()
    anatomy = resolve_anatomy(
        session,
        url,
        preset or os.environ.get("RIT_ANATOMY_PRESET", "__primary__"),
        overrides={"roots": [{"name": ROOT_NAME, "windows": "C:/resolver_it_work", "linux": work_root, "darwin": work_root}]},
    )
    token = uuid.uuid4().hex[:8]
    info = create_project(session, url, anatomy, name=project or f"resolverit_{token}", code=f"rit{token[:5]}")
    name = info.project_name
    try:
        return _publish_closure(session, url, info, anatomy, work_root=work_root, asset_count=asset_count)
    except BaseException:
        # The session fixture never reaches its teardown when seeding fails; clean up here.
        delete_project(session, url, name)
        shutil.rmtree(Path(work_root) / name, ignore_errors=True)
        raise


def teardown(manifest: Manifest) -> None:
    session, url = connect()
    delete_project(session, url, manifest.project)
    shutil.rmtree(Path(manifest.work_root) / manifest.project, ignore_errors=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("seed")
    s.add_argument("--assets", type=int, default=24)
    s.add_argument("--work-root", default=os.environ.get("RIT_WORK_ROOT", "/data/work"))
    s.add_argument("--project")
    s.add_argument("--preset", help="anatomy preset (default: RIT_ANATOMY_PRESET or __primary__)")
    t = sub.add_parser("teardown")
    t.add_argument("manifest", help="manifest JSON written by `seed`")
    args = parser.parse_args()

    if args.cmd == "seed":
        manifest = seed(args.work_root, args.assets, args.project, args.preset)
        json.dump(asdict(manifest), sys.stdout, indent=2)
        print()
    else:
        teardown(Manifest(**json.loads(Path(args.manifest).read_text(encoding="utf-8"))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
