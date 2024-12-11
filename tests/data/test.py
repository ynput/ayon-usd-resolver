from pxr import Usd, Sdf
import base64
from datetime import datetime


def getAttrByKey(prim: Usd.Prim, key: str) -> Usd.Attribute:
    for attr in prim.GetAttributes():
        attr_name = attr.GetName()
        if "Ayon:RedirectionData:" + key in attr_name:
            return attr


def getRedirectionData(redirectionPrim: Usd.Prim) -> dict:
    redirectionData = {}
    for attr in prim.GetAttributes():
        attr_name = attr.GetName()
        if "Ayon:RedirectionData:" in attr_name:
            redirectionData[attr_name.split(":")[-1]] = attr.Get()
    return redirectionData


base_stage = Usd.Stage.CreateNew("baseStage.usda")
prim = base_stage.DefinePrim(
    "/AyonData/RedirectionData",
)

keys_attr = prim.CreateAttribute(
    f"Ayon:RedirectionData:KeyA",
    Sdf.ValueTypeNames.String,
).Set("ValA")
start = datetime.now()
for i in range(10):
    keys_attr = prim.CreateAttribute(
        f"Ayon:RedirectionData:Key{i}",
        Sdf.ValueTypeNames.String,
    ).Set(f"Val{i}")


base_stage.Save()
print("Base Stage:")
# print(base_stage.GetRootLayer().ExportToString())


overlay_stage = Usd.Stage.CreateNew("overlayStage.usda")
overlay_stage.GetRootLayer().subLayerPaths.append("baseStage.usda")

overlay_prim = overlay_stage.GetPrimAtPath("/AyonData/RedirectionData")

getAttrByKey(overlay_prim, "KeyA").Set("ValB")

overlay_stage.Save()
print("\nOverlay Stage:")
# print(getAttrByKey(overlay_prim, "KeyA").Get())
print(getRedirectionData(overlay_prim))
