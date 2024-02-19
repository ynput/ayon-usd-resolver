import random
from pxr import Ar

print("Resolver General Test")



num_random_numbers = 5

Ar.SetPreferredResolver("AyonUsdResolver")
resolver = Ar.GetResolver()





# Generate and print random numbers
for i in range(num_random_numbers):
	rpath = f"ayon://Usd_Base/trees?product=usdtree_{i}&version=*&representation=usd"
	print("Requested Path:")
	print(rpath)
	
	resolved_path = resolver.Resolve(rpath)
	print("Resolved Path:")
	print(resolved_path)
