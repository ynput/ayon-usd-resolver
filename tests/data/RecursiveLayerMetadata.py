from pxr import Usd, Vt, Sdf
import os

Entry = Usd.Stage.CreateNew("Entry.usda")

Entry.GetRootLayer().customLayerData = {
    "AyonUsdResolverRedirectionFile": Vt.Token(
        f"{os.path.abspath('RedirectionB.json')}"
    ),
}

prim = Entry.DefinePrim("/test", "Xform")
prim.GetReferences().AddReference("fileA.usda")

Entry.Save()


RedirectionA = Sdf.Layer.CreateNew("RedirectionA.usda")

RedirectionA.customLayerData = {
    "AyonUsdResolverRedirectionData": {
        "fileA.usda": Vt.Token("redirectionDataA.usda"),
    }
}
RedirectionA.Save()


RedirectionB = Sdf.Layer.CreateNew("RedirectionB.usda")
RedirectionB.subLayerPaths.append("RedirectionA.usda")

RedirectionB.customLayerData = {
    "AyonUsdResolverRedirectionData": {
        "fileA.usda": Vt.Token(f'{os.path.abspath("sphere.usda")}'),
    }
}
RedirectionB.Save()
