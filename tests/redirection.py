# TODO implement a system to setup a fake server to test this funciotns against an actuall server
from pxr import Usd, Ar, Sdf, Vt, UsdGeom
import json
import os

from usdAssetResolver import AyonUsdResolver
from collections import namedtuple
import pytest


def wrtie_empty_rdf(path):
    with open(path, "w") as json_file:
        json.dump({"subLayers": [], "data": {}}, json_file, indent=4)


@pytest.fixture
def setup_usd_file_with_empty_rdf(tmp_path):
    rdf_name = "EmptyRdf.json"
    wrtie_empty_rdf(tmp_path / rdf_name)

    usd_name = "UsdWtihMissingRef.usda"
    stage = Usd.Stage.CreateNew(str(tmp_path / usd_name))

    stage.GetRootLayer().customLayerData = {
        "AyonUsdResolverRedirectionFile": Vt.Token(str(tmp_path / rdf_name))
    }

    prim = stage.DefinePrim("/test", "Xform")
    prim.GetReferences().AddReference("missingRef.usda")
    stage.Save()

    sphere = "sphere.usda"
    sphereUsd = Usd.Stage.CreateNew(str(tmp_path / sphere))
    mesh = UsdGeom.Mesh.Define(sphereUsd, "/sopcreate1/mesh_0")
    sphereUsd.SetDefaultPrim(sphereUsd.GetPrimAtPath("/sopcreate1"))
    sphereUsd.Save()

    SetupData = namedtuple("SetupData", ["UsdStagePath", "RdfPath"])
    yield SetupData(UsdStagePath=tmp_path / usd_name, RdfPath=tmp_path / rdf_name)


def create_usd_with_missing_reff(tmp_path, rdf_name) -> os.path:
    usd_name = "UsdWtihMissingRef.usda"
    stage = Usd.Stage.CreateNew(str(tmp_path / usd_name))

    stage.GetRootLayer().customLayerData = {
        "AyonUsdResolverRedirectionFile": Vt.Token(str(tmp_path / rdf_name))
    }

    prim = stage.DefinePrim("/test", "Xform")
    prim.GetReferences().AddReference("missingRef.usda")
    stage.Save()
    return os.path.join(tmp_path, usd_name)


def create_sphere_usd(tmp_path) -> os.path:
    sphere = "sphere.usda"
    sphereUsd = Usd.Stage.CreateNew(str(tmp_path / sphere))
    mesh = UsdGeom.Mesh.Define(sphereUsd, "/sopcreate1/mesh_0")
    sphereUsd.SetDefaultPrim(sphereUsd.GetPrimAtPath("/sopcreate1"))
    sphereUsd.Save()

    return os.path.join(tmp_path, sphere)


def test_for_correct_resolver(tmp_path):
    resolver = Ar.GetUnderlyingResolver()
    assert "AyonUsdResolver.Resolver" in str(resolver)


# TODO this only works with the right server
def test_simple_resolve(tmp_path):
    resolver = Ar.GetUnderlyingResolver()
    path = resolver.Resolve(
        "ayon+entity://Usd_Testing/Hou_Ues_Exp/Test_Assets/assetA?product=usdMain&version=v002&representation=usd"
    )
    # TODO this should work in a way where it tests if the file is on disk
    assert path


# TODO test all the other functions
