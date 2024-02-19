#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_DIR=$SCRIPT_DIR/dist

#---------- Set Usd/Python Varibles for Resolver Load
export USD_ASSET_RESOLVER=$SCRIPT_DIR
# export TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER
export LD_LIBRARY_PATH=$SCRIPT_DIR/ayonUsdResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME
export PYTHONPATH=$SCRIPT_DIR/ayonUsdResolver/lib/python:


#set those env varibles if your not starting the resolver form ayon env. 
#export "AYON_SITE_ID=groovy-amiable-reindeer"
#export AYON_API_KEY="0d3d1ac3d275db81808059e8782dc40bdb15b44cef09d32ce576d1febc0bcb82"
#export AYON_SERVER_URL="http://192.168.178.42:5000"



pushd /opt/hfs20.0 && source houdini_setup && popd

houdini -foreground
