import random
from pxr import Ar
from usdAssetResolver import AyonUsdResolver


Ar.SetPreferredResolver("AyonUsdResolver")
resolver = Ar.GetResolver()
context = AyonUsdResolver.ResolverContext()

#rpath = f"ayon://Usd_Base/trees?product=usdtree_4&version=*&representation=usd"
#rpath = f"ayon://Usd_Base/UsdTesting?product=modelMain_2&version=*&representation=obj"
rpath = f"ayon://Usd_Base/UsdTesting?product=usdUsdTest_214&version=latest&representation=usd"
print("\033[95m" + "Requested Path:" + "\033[0m")
print(rpath)

print("\033[95m" + "Resolved Path:" + "\033[0m")
resolved_path = resolver.Resolve(rpath)
print(resolved_path)

print("\033[95m" + "Resolved Path:" + "\033[0m")
resolved_path = resolver.Resolve(rpath)
print(resolved_path)

print("\033[95m" + "Resolving after Deleting Cache Entry: " + "\033[0m")
context.deletFromCache(rpath)
resolved_path = resolver.Resolve(rpath)
print("\033[95m" + "Resolved Path:" + "\033[0m")
print(resolved_path)
