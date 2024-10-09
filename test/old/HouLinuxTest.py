import random
from pxr import Ar
from usdAssetResolver import AyonUsdResolver
import time

Ar.SetPreferredResolver("AyonUsdResolver")
resolver = Ar.GetResolver()
context = AyonUsdResolver.ResolverContext()

#rpath = f"ayon://Usd_Base/trees?product=usdtree_4&version=*&representation=usd"
#rpath = f"ayon://Usd_Base/UsdTesting?product=modelMain_2&version=*&representation=obj"

print()
startGlobal = time.time()

start = time.time()
apath = f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_214&version=latest&representation=usd"
print("\033[95m" + "Requested Path:" + "\033[0m")
print(apath)
print("\033[95m" + "Resolved Path:" + "\033[0m")
resolved_path = resolver.Resolve(apath)
print(resolved_path)
end = time.time()
print(end - start)

start = time.time()
rpath = f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_218&version=latest&representation=usd"
print("\033[95m" + "Resolved Path:" + "\033[0m")
resolved_path = resolver.Resolve(rpath)
print(resolved_path)
end = time.time()
print(end - start)

start = time.time()
rpath = f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_24&version=latest&representation=usd"
print("\033[95m" + "Resolved Path:" + "\033[0m")
resolved_path = resolver.Resolve(rpath)
print(resolved_path)
end = time.time()
print(end - start)

start = time.time()
rpath = f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_213&version=latest&representation=usd"
print("\033[95m" + "Resolved Path:" + "\033[0m")
resolved_path = resolver.Resolve(rpath)
print(resolved_path)
end = time.time()
print(end - start)

start = time.time()
rpath = f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_215&version=latest&representation=usd"
print("\033[95m" + "Resolved Path:" + "\033[0m")
resolved_path = resolver.Resolve(rpath)
print(resolved_path)
end = time.time()
print(end - start)

start = time.time()
print("\033[95m" + "Resolved Path:" + "\033[0m")
resolved_path = resolver.Resolve(apath)
print(resolved_path)
end = time.time()
print(end - start)


start = time.time()
print()
print("\033[95m" + "Resolving after Deleting Cache Entry: " + "\033[0m")
context.deleteFromCache(rpath)
resolved_path = resolver.Resolve(rpath)
print("\033[95m" + "Resolved Path:" + "\033[0m")
print(resolved_path)
end = time.time()
print(end - start)

print()
print("all time")
endGlobal = time.time()
print(endGlobal - startGlobal)
