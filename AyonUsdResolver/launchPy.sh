#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

#---------- Set Usd/Python Variables for Resolver Load
export USD_ASSET_RESOLVER=$SCRIPT_DIR
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER
export LD_LIBRARY_PATH=$SCRIPT_DIR/ayonUsdResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME
export PYTHONPATH=$SCRIPT_DIR/ayonUsdResolver/lib/python:

export AYON_API_KEY="0d3d1ac3d275db81808059e8782dc40bdb15b44cef09d32ce576d1febc0bcb82"
export AYON_SERVER_URL="http://192.168.178.42:5000"

# Source Houdini setup script
pushd /opt/hfs20.0 && source houdini_setup && popd

# Start Houdini with Hython
hython << EOF
import random
from pxr import Ar

num_random_numbers = 5

Ar.SetPreferredResolver("AyonUsdResolver")
resolver = Ar.GetResolver()





# Generate and print random numbers
for i in range(num_random_numbers):
	rpath = f"ayon://Usd_Base/trees?product=usdtree_{i}&version=*&representation=usd"
	print("\033[95m" + "Requested Path:" + "\033[0m")
	print(rpath)
	
	resolved_path = resolver.Resolve(rpath)
	print("\033[95m" + "Resolved Path:" + "\033[0m")
	print(resolved_path)


EOF

