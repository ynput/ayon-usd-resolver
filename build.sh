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


#------------------ Development Env Wars ----------------- 



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
HOU_START=0
HOU_VER=20.0 # Set the houdini version // needs to be the same as the hfs foulder version in your install

if [ "$OS" == "Linux" ]; then 
  export HFS="/opt/hfs${HOU_VER}"
fi



#-------------- Debug Over Writes Main
if [ "$DEBUG" -eq 1 ]; then
  DEV=1
  source devOpVars.sh
  echo " dev var is set to $DEV"

  #TODO function that looks if houdini is the SOFTWARE_TARGET
  HOU_START=1
fi

#------------- Set DevOps Varibles 

export DEV=$DEV


#--------------- Build Execution ------------------------ 
if [ "$CLEAN_BUILD" -eq 1 ]; then
  echo "Clean build is activated"
  rm -rf bin
  rm -rf build
  mkdir bin
  mkdir build
fi


set -e # Exit on error
cmake . -B build -DDEV=$DEV -DJTRACE=$JTRACE


if [ "$CLEAN_BUILD" -eq 1 ]; then 
  cmake --build build --clean-first --parallel
else
  cmake --build build --parallel
fi

cmake --install build   
#----------- Copy / Iinstall Setup------------------------------- 
#TODO move this to use Cmake -Install 
echo ""
RESOLVER_EXEC_PATH="$SCRIPT_DIR/AyonUsdResolver/ayonUsdResolver/lib/ayonUsdResolver.so"
RESOLVER_PY_PATH="$SCRIPT_DIR/AyonUsdResolver/ayonUsdResolver/lib/python/_ayonUsdResolver.so"

# Remove Exec build
#if [ -e "$RESOLVER_EXEC_PATH" ]; then
#    rm "$RESOLVER_EXEC_PATH"
#    echo "Removed File $RESOLVER_EXEC_PATH."
#else
#    echo "File $RESOLVER_EXEC_PATH does not exist."
#fi
# Remove Py build
#if [ -e "$RESOLVER_PY_PATH" ]; then
#    rm "$RESOLVER_PY_PATH"
#    echo "Removed File $RESOLVER_PY_PATH."
#else
#    echo "File $RESOLVER_PY_PATH does not exist."
#fi
#Copy Newlie build files into there respective paths 
#cp $BUILD_DIR/src/AyonUsdResolver/ayonUsdResolver.so $RESOLVER_EXEC_PATH
#echo "copyied build .so file to $RESOLVER_EXEC_PATH"
#cp $SCRIPT_DIR/build/src/AyonUsdResolver/_ayonUsdResolver.so $RESOLVER_PY_PATH
#echo "copyied build .so file to $RESOLVER_PY_PATH"


#-------------- Running Tests --------------- 
#TODO implement Tests 

if [ "$TEST" -eq 1 ]; then 
  ctest -VV --test-dir build
fi

# if [ "$HOU_START" -eq 1 ]; then
#   echo "statring houdini"
#   cd $SCRIPT_DIR/AyonUsdResolver
#   ./launch.sh
# fi
