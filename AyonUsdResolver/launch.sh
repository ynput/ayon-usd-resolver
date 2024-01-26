#!/bin/bash
# Setup environment

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"


export USD_ASSET_RESOLVER=$SCRIPT_DIR
export TF_DEBUG=CACHEDRESOLVER_RESOLVER_CONTEXT
# export TF_DEBUG=CACHEDRESOLVER_RESOLVER
export LD_LIBRARY_PATH=$SCRIPT_DIR/cachedResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/cachedResolver/resources:$PXR_PLUGINPATH_NAME



export PYTHONPATH=$SCRIPT_DIR/cachedResolver/lib/python:

# Launch Houdini
pushd /opt/hfs20.0.547 && source houdini_setup && popd
#houdini -foreground -desktop solaris
houdini -foreground /home/lyonh/Documents/dell/test.hiplc
# houdini -foreground -desktop solaris /home/lyonh/Documents/resolver_test_usd_things/resolver_testing.hiplc

