#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_DIR=$SCRIPT_DIR/Resolvers/LinuxPy310Houdini20

#---------- Set Usd/Python Variables for Resolver Load
export USD_ASSET_RESOLVER=$SCRIPT_DIR
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT #AYONUSDRESOLVER_RESOLVER_CONTEXT #AYONUSDRESOLVER_RESOLVER
export LD_LIBRARY_PATH=$SCRIPT_DIR/ayonUsdResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME
export PYTHONPATH=$SCRIPT_DIR/ayonUsdResolver/lib/python:

export AYON_API_KEY="b0ce104dba031e0aef71781a40b50c3cf086e4e485ec6967444b5bf48559fd25"
export AYON_SERVER_URL="http://192.168.178.42:5000"

export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="ON"
export AYONLOGGERFILEPOS="LoggingFiles"

# Source Houdini setup script
pushd /opt/hfs20.0.590 && source houdini_setup && popd

#19.5.805
#20.0.590

# Start Houdini with Hython
hython << EOF
import random
from pxr import Ar
from usdAssetResolver import AyonUsdResolver
num_random_numbers = 1

Ar.SetPreferredResolver("AyonUsdResolver")
resolver = Ar.GetResolver()
context = AyonUsdResolver.ResolverContext()


for i in range(num_random_numbers):
	rpath = f"ayon://Usd_Base/trees?product=usdtree_1&version=*&representation=usd"
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



EOF

