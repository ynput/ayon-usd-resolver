#!/bin/bash
#----------- Build Script Paths ------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR=$SCRIPT_DIR/build

#----------- Check for os Type ------------------------
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="Linux"
    VERSION=$(cat /etc/os-release | grep "VERSION_ID" | cut -d '"' -f 2)
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="MacOS"
    VERSION=$(sw_vers -productVersion)
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    OS="Windows"
    VERSION=$(ver)
else
    OS="Unknown"
    VERSION="Unknown"
fi
echo "Detected OS: $OS"
echo "OS Version: $VERSION"

#---------- Build Script Codes ---------------
DEBUG=0
CLEAN_BUILD=0
DEV=0
JTRACE=0

#---------- Build Choise Definitions --------------
SOFTWARE_TARGET="" 
TEST=0

#----------------- Build Script Statup Tests ------------------
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

#------------------- Houdini Config ------------------------
HOU_VER=19.5.805 # Set the houdini version // needs to be the same as the hfs foulder version in your install

#19.5.805
#20.0.590


if [ "$OS" == "Linux" ]; then 
  export HFS="/opt/hfs${HOU_VER}"
fi


#-------------- Debug Over Writes Main
if [ "$DEBUG" -eq 1 ]; then
  DEV=1
  source devOpVars.sh
  echo " dev var is set to $DEV"
fi

#------------- Set DevOps Varibles 

export DEV=$DEV

#--------------- Build Execution ------------------------ 
if [ "$CLEAN_BUILD" -eq 1 ]; then
  echo "Clean build is activated"
  rm -rf bin
  rm -rf build
  rm -rf Resolvers
  mkdir bin
  mkdir build
  mkdir Resolvers
fi


set -e # Exit on error
cmake . -B build -DDEV=$DEV -DJTRACE=$JTRACE


if [ "$CLEAN_BUILD" -eq 1 ]; then 
  cmake --build build --clean-first
else
  cmake --build build
fi

cmake --install build   
