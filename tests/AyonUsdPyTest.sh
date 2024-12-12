#!/bin/bash

python Project.py --execSingleStage "Build Resolvers" --Target AyonUsd --BuildType Debug
cp build/AyonUsd_Linux/compile_commands.json build/compile_commands.json
#
SCRIPT_DIR="$(pwd)"/Resolvers/AyonUsd_Linux

USD_INSTALL_DIR="/home/ynput/Documents/GitHub/Lyonh/AyonDev/ayon_usd/AyonUsdBin/usd-24.03_linux_py39/24.03"

export PATH="$USD_INSTALL_DIR/bin:$PATH"
export LD_LIBRARY_PATH="$USD_INSTALL_DIR/lib:$LD_LIBRARY_PATH"
export PYTHONPATH="$USD_INSTALL_DIR/lib/python:$PYTHONPATH"



export USD_ASSET_RESOLVER=${USD_ASSET_RESOLVER:+$USD_ASSET_RESOLVER:}$SCRIPT_DIR
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}$SCRIPT_DIR/ayonUsdResolver/lib
export PXR_PLUGINPATH_NAME=${PXR_PLUGINPATH_NAME:+$PXR_PLUGINPATH_NAME:}$SCRIPT_DIR/ayonUsdResolver/resources
export PYTHONPATH=${PYTHONPATH:+$PYTHONPATH:}$SCRIPT_DIR/ayonUsdResolver/lib/python

# export TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT 
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER 

export AYON_SERVER_URL="http://192.168.47.131:5000"
export AYON_API_KEY="1d666ff2101dd7564b4bdbbd157d4c7ff813f1c64269bb1329e21672cc740f03"
export AYON_PROJECT_NAME="Usd_Testing"
export AYON_SITE_ID="hospitable-dynamic-wasp"

export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="OFF"
export AYONLOGGERFILEPOS="LoggingFiles"

source tests/testVenv/bin/activate


pytest -s tests/Redirectoin_t.py
# gdb --args python -m pytest -s tests/Redirectoin_t.py::test_redirection_add_layer
