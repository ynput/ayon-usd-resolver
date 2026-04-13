#!/bin/bash

python Project.py --execSingleStage "Build Resolvers" --Target AyonUsd



SCRIPT_DIR=$(pwd)/Resolvers/AyonUs_Linux


export USD_ASSET_RESOLVER=$SCRIPT_DIR
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT 
export LD_LIBRARY_PATH=$SCRIPT_DIR/ayonUsdResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME
export PYTHONPATH=$SCRIPT_DIR/ayonUsdResolver/lib/python:


#export AYON_API_KEY=""
#export AYON_SERVER_URL=""
#export AYON_SERVER_URL=""


export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="OFF"
export AYONLOGGERFILEPOS="LoggingFiles"

# Source Houdini setup script
pushd /opt/hfs20.0.590 && source houdini_setup && popd


# Start Houdini with Hython
hython test/HouLinuxTest.py
