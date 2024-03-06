#!/bin/bash
#----------- Build Script Paths ------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"


#------------------- Houdini Config ------------------------
#HOU_VER={Houdini-Version} # Set the Houdini version // needs to be the same as the hfs fouler version in your install e.g 20.0.630

#export INSTALLNAME="{CompilePlugin Name Overwrite}"
#export COMPILEPLUGIN="{CompilePlugin Name and Path}" #e.g HouLinux/LinuxPy310Houdini20
#HOUDINI_INSTALL_DIR="{Houdini install directory without Houdini number}" # on this variable you can set your Houdini install fouler. 
#just remember that you want the fouler without the number so that the HFS export can construct it correctly. 
# we do it this way to make batch building the resolver easier. 

if [ -z "$HOUDINI_INSTALL_DIR" ]; then
    HOUDINI_INSTALL_DIR="/opt/hfs" #if you didn't set the Install dir variable we assume your Houdini is installed in the default directory
fi

export HFS="${HOUDINI_INSTALL_DIR}${HOU_VER}" # this auto constructs the default install path for most Houdini installs under Linux. if you moved your 
#---------- Build Script Codes ---------------
DEBUG=0
CLEAN_BUILD=0
DEV=0
JTRACE=0


#----------------- Build Script Start Up Tests ------------------
if [ "$1" == "Debug" ]; then
  echo "Build running in debug mode"
  DEBUG=1
fi
if [[ " $@ " =~ " Clean " ]]; then
  echo "Running Clean Build"
  CLEAN_BUILD=1
fi
if [[ " $@ " =~ " Debug " && " $@ " =~ " Clean " ]]; then
  echo "Running Debug and Clean Build"
  DEBUG=1
  CLEAN_BUILD=1
fi


if [ "$DEBUG" -eq 1 ]; then
  DEV=1
  source devOpVars.sh
  echo " dev var is set to $DEV"
fi

if [ "$CLEAN_BUILD" -eq 1 ]; then
  echo "Clean build is activated"
  rm -rf build
  rm -rf Resolvers
  mkdir build
  mkdir Resolvers
fi

#----------------- cmake Commands ------------------
set -e # Exit on error
cmake . -B build -DDEV=$DEV -DJTRACE=$JTRACE
if [ "$CLEAN_BUILD" -eq 1 ]; then 
  cmake --build build --clean-first
else
  cmake --build build
fi
cmake --install build   
