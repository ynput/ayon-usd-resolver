from pxr import Usd, Vt, Sdf
import os

# Step 1: Open the existing USD stage
AyonLight = Usd.Stage.Open("AyonLight.usda")

AyonLight.GetRootLayer().customLayerData = {
    "AyonUsdResolverRedirectionLayerFile": Vt.Token("redirectionData.usda")
}

objA = AyonLight.DefinePrim("/test/obj", "Xform")
objA.GetReferences().AddReference("file.usda")
AyonLight.GetRootLayer().Save()


redirectionData = Usd.Stage.CreateNew("redirectionData.usda")


redirectionData.GetRootLayer().customLayerData = {
    "file.usda": Vt.Token(os.path.abspath("sphere.usda"))
}

redirectionData.GetRootLayer().Save()
