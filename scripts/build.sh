#!/bin/bash
set -e # Exit on error
cd ../ #move to root dir
#----------- Build Script Paths ------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo $SCRIPT_DIR
if [ -z "$HOUDINI_INSTALL_DIR" ]; then
    HOUDINI_INSTALL_DIR="/opt/hfs" # if you didn't set the Install dir variable we assume your Houdini is installed in the default directory
fi
export HFS="${HOUDINI_INSTALL_DIR}${HOU_VER}" 

CLEAN_BUILD=0
DEV=0
JTRACE=0

if [[ " $@ " =~ " Debug " ]]; then
  echo "Build running in debug mode"
  DEV=1

  source devOpVars.sh
  echo " dev var is set to $DEV"
fi

if [[ " $@ " =~ " Clean " ]]; then
  echo "Running Clean Build"
  CLEAN_BUILD=1

  rm -rf build
  rm -rf Resolvers/${COMPILEPLUGIN}
  mkdir build
fi



cmake . -B build -DDEV=$DEV -DJTRACE=$JTRACE
if [ "$CLEAN_BUILD" -eq 1 ]; then
  cmake --build build --clean-first
else
  cmake --build build
fi
cmake --install build
