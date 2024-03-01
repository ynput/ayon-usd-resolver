#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_DIR=$SCRIPT_DIR/Resolvers/HouLinux/LinuxPy310Houdini20


#---------- Set Usd/Python Variables for Resolver Load
export USD_ASSET_RESOLVER=$SCRIPT_DIR
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT #AYONUSDRESOLVER_RESOLVER_CONTEXT #AYONUSDRESOLVER_RESOLVER
export LD_LIBRARY_PATH=$SCRIPT_DIR/ayonUsdResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME
export PYTHONPATH=$SCRIPT_DIR/ayonUsdResolver/lib/python:

#export AYON_API_KEY="7cefc33b3e47ef9804034dc6adaca9e92a9ceed03d341bbec46a64b09caaae7d"
#export AYON_SERVER_URL="https://usd.ayon.app"

export AYON_API_KEY="4994faaec0b431dcf75f62c738ecbe389e06184da61e027b89ef958f21b2d1ed"
#export AYON_SERVER_URL="http://192.168.178.42:5000"
export AYON_SERVER_URL="https://ayontest.open-planck.de"


export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="ON"
export AYONLOGGERFILEPOS="LoggingFiles"

# Source Houdini setup script
pushd /opt/hfs20.0.590 && source houdini_setup && popd

#19.0.720
#19.5.805
#19.5.900
#20.0.590
#20.0.630

# Start Houdini with Hython
hython test/Debug.py
