#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_DIR=$SCRIPT_DIR/Resolvers

#---------- Set Usd/Python Variables for Resolver Load
export USD_ASSET_RESOLVER=$SCRIPT_DIR
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER
export LD_LIBRARY_PATH=$SCRIPT_DIR/ayonUsdResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME
export PYTHONPATH=$SCRIPT_DIR/ayonUsdResolver/lib/python:

export AYON_API_KEY="fa12949bbf12ee79f013a8713e7a13e4c3edb42efa539f491334a81294179e36"
export AYON_SERVER_URL="http://192.168.178.42:5000"

export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="ON"
export AYONLOGGERFILEPOS="LoggingFiles"

# Source Houdini setup script
pushd /opt/hfs19.5.805 && source houdini_setup && popd

#19.5.805
#20.0.590

# Start Houdini with Hython
hython << EOF
import random
from pxr import Ar

num_random_numbers = 1

Ar.SetPreferredResolver("AyonUsdResolver")
resolver = Ar.GetResolver()





# Generate and print random numbers
for i in range(num_random_numbers):
	rpath = f"ayon://Usd_Base/trees?product=usdtree_1&version=*&representation=usd"
	#rpath = f"ayon://Usd_Base/trees?product=usdtree_{i}&version=*&representation=usd"
	print("\033[95m" + "Requested Path:" + "\033[0m")
	print(rpath)
	
	resolved_path = resolver.Resolve(rpath)
	print("\033[95m" + "Resolved Path:" + "\033[0m")
	print(resolved_path)


EOF

