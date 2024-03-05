import random
import hou
from pxr import Ar
resolver = Ar.GetResolver()  # under windows it is necessary to trigger usd initialisation in Houdini before you import AyonUsdResolver
from usdAssetResolver import AyonUsdResolver


rpath = f"ayon://Usd_Base/UsdTesting?product=usdUsdTest_214&version=latest&representation=usd"
print("Requested Path:")
print(rpath)

resolved_path = resolver.Resolve(rpath)
print("Resolved Path:")
print(resolved_path)

resolved_path = resolver.Resolve(rpath)
print("Resolved Path:")
print(resolved_path)

context = AyonUsdResolver.ResolverContext()
print("Resolving after Deleting Cache Entry: ")
context.deletFromCache(rpath)
resolved_path = resolver.Resolve(rpath)
print("Resolved Path:")
print(resolved_path)