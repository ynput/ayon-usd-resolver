#!/bin/bash

python Project.py --execSingleStage "Build Resolvers" --Target AyonUsd --BuildType Debug
#
#
SCRIPT_DIR="$(pwd)"/Resolvers/AyonUsd_Linux

export USD_ASSET_RESOLVER=${USD_ASSET_RESOLVER:+$USD_ASSET_RESOLVER:}$SCRIPT_DIR
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}$SCRIPT_DIR/ayonUsdResolver/lib
export PXR_PLUGINPATH_NAME=${PXR_PLUGINPATH_NAME:+$PXR_PLUGINPATH_NAME:}$SCRIPT_DIR/ayonUsdResolver/resources
export PYTHONPATH=${PYTHONPATH:+$PYTHONPATH:}$SCRIPT_DIR/ayonUsdResolver/lib/python

# export TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT 
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER 

export AYON_SERVER_URL="http://192.168.47.131:5000"
export AYON_API_KEY="60cf27272f71b85bca6ebcf98ded99ffe379094d7f55a457331df8bb6dfca31d"
export AYON_PROJECT_NAME="Usd_Testing"

export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="OFF"
export AYONLOGGERFILEPOS="LoggingFiles"

source tests/testVenv/bin/activate


pytest -s tests/Redirectoin_t.py::test_create_usd_and_add_rdf_later
# gdb --args python -m pytest -s tests/Redirectoin_t.py::test_redirection_add_layer
