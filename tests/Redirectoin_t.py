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


@pytest.fixture
def setup_usd_file_with_rdf(tmp_path):
    rdf_name = "Rdf.json"

    with open(tmp_path / rdf_name, "w") as json_file:
        json.dump(
            {"subLayers": [], "data": {"missingRef.usda": "./sphere.usda"}},
            json_file,
            indent=4,
        )

    base_usd_path = create_usd_with_missing_reff(tmp_path, rdf_name)
    sphere_usd_path = create_sphere_usd(tmp_path)

    SetupData = namedtuple("SetupData", ["UsdStagePath", "RdfPath"])
    yield SetupData(UsdStagePath=base_usd_path, RdfPath=sphere_usd_path)


@pytest.fixture
def setup_and_reset_RDFB_File(tmp_path):
    rdfB_path = os.path.join(os.path.dirname(__file__), "data/RedirectionB.json")
    with open(rdfB_path, "w") as json_file:
        json.dump({"subLayers": [], "data": {}}, json_file, indent=4)

    yield

    rdfB_path = os.path.join(os.path.dirname(__file__), "data/RedirectionB.json")
    with open(rdfB_path, "w") as json_file:
        json.dump({"subLayers": [], "data": {}}, json_file, indent=4)


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


def test_redirection_path(setup_usd_file_with_rdf):
    stage = Usd.Stage.Open(str(setup_usd_file_with_rdf.UsdStagePath))
    assert "mesh_0" in stage.ExportToString()


def test_redirection_add_layer(setup_and_reset_RDFB_File):

    rootLayerPath = os.path.join(os.path.dirname(__file__), "data/EntryB.usda")
    stage = Usd.Stage.Open(rootLayerPath)

    root_layer = stage.GetRootLayer()

    # we store the rdf file location for a given stage in the usd stage for sinplicity on the resoler side.
    rdf_path = root_layer.customLayerData["AyonUsdResolverRedirectionFile"]

    # the getRdFile function cant figure out ancored paths and will constrcut the abs path based on the current chroot dir of the process. for this metter we need to resolve to an abs path before hand
    rdf_path = os.path.abspath(
        os.path.join(os.path.dirname(root_layer.realPath), rdf_path)
    )

    # the getRdFile function returns an instance of an rdf file based on a given file path. it is adviced to use an abs file path because the resolver will internally also resolve to an abs path and the getRdFile func dose not care anout if the file exists or not.
    rdf_file = AyonUsdResolver.getRdFile(rdf_path)

    # adding a new layer to an rdfFile will reload the file internally but you still need to reload the stage (or the layer that introduces the assetPath that needs redirectoin) over all its adviced to just reload when every do change redirection data because you might bet dirtry data on the rdf
    redirection_file = os.path.join(os.path.dirname(__file__), "data/RedirectionA.json")
    rdf_file.addLayer(redirection_file, True)
    stage.Reload()

    m0 = stage.GetPrimAtPath("/test/mesh_0")
    assert m0, "the expected prim(/test/mesh_0) cant be found, redirectoin invalid"


def test_redirection_add_redirection(setup_usd_file_with_empty_rdf):
    stage = Usd.Stage.Open(str(setup_usd_file_with_empty_rdf.UsdStagePath))

    root_layer = stage.GetRootLayer()
    rdf_path = root_layer.customLayerData["AyonUsdResolverRedirectionFile"]
    rdf_path = os.path.abspath(
        os.path.join(os.path.dirname(root_layer.realPath), rdf_path)
    )
    rdf_file = AyonUsdResolver.getRdFile(rdf_path)

    rdf_file.addRedirection("missingRef.usda", "./sphere.usda")
    stage.Reload()

    m0 = stage.GetPrimAtPath("/test/mesh_0")
    assert m0, "the expected prim(/test/mesh_0) cant be found, redirectoin invalid"


def test_create_usd_and_add_rdf_later(tmp_path):
    usd_path = tmp_path / "usd.usda"

    rdf_name = "test_rdf.json"
    rdf_path = tmp_path / rdf_name
    wrtie_empty_rdf(rdf_path)
    sphere_usd = create_sphere_usd(tmp_path)

    stage = Usd.Stage.CreateNew(str(usd_path))

    res = Ar.GetResolver()
    ctx = res.GetCurrentContext()

    raise RuntimeError(ctx.IsEmpty())

    rdf = AyonUsdResolver.getRdFile(str(rdf_path))
    rdf.addRedirection("missingRef.usda", "./sphere.usda")

    print("\n" * 10, "set rdf")
    res.setRedirectionFileForCtx(str(rdf_path))
    rdfB = res.GetRedirectionFile()

    print("\n" * 10, "adding")
    prim = stage.DefinePrim("/test", "Xform")
    prim.GetReferences().AddReference("missingRef.usda")

    assert False, stage.ExportToString()
