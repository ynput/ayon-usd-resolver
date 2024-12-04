import random
from pxr import Ar
from usdAssetResolver import AyonUsdResolver
import time

Ar.SetPreferredResolver("AyonUsdResolver")
resolver = Ar.GetResolver()
context = AyonUsdResolver.ResolverContext()


def resolve_path(AssetPath):

    print()
    print("-"*80)
    print("\033[95m" + "Requested Path:" + "\033[0m")
    print(AssetPath)
    start = time.time()
    resolved_path = resolver.Resolve(AssetPath)
    end = time.time()
    print("\033[95m" + "Resolved Path:" + "\033[0m")
    print(resolved_path)
    print("\033[95m" + "Execution Time:" + "\033[0m")
    print(end - start, " /s")
    print("-"*80)
    print()

startGlobal = time.time()

paths = [f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_214&version=latest&representation=usd" ,
f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_218&version=latest&representation=usd",
f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_24&version=latest&representation=usd",
f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_213&version=latest&representation=usd",
f"ayon+entity://Usd_Base/UsdTesting?product=usdUsdTest_215&version=latest&representation=usd",]


print()
print("#"*80)
print("\033[95m" + "Test" + "\033[0m")

for path in paths:
	resolve_path(path)


print()
print("all time")
endGlobal = time.time()
print(endGlobal - startGlobal)

print("#"*80)
print()
