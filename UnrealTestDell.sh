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

https://docs.unrealengine.com/5.3/en-US/building-unreal-engine-from-source/UnrealEditor

