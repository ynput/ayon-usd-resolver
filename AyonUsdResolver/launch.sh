#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

#---------- Set Usd/Python Varibles for Resolver Load
export USD_ASSET_RESOLVER=$SCRIPT_DIR
export LD_LIBRARY_PATH=$SCRIPT_DIR/ayonUsdResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME
export PYTHONPATH=$SCRIPT_DIR/ayonUsdResolver/lib/python:

#---------- Set Debug Code
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT
# export TF_DEBUG=AYONUSDRESOLVER_RESOLVER

#---------- Launch Houdini
cd /opt/hfs20.0
source houdini_setup
houdini -foreground

#houdini -foreground /home/lyonh/Documents/dell/test.hiplc
#houdini -foreground -desktop solaris /home/lyonh/Documents/resolver_test_usd_things/resolver_testing.hiplc

