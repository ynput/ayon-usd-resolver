#---------- Set Usd/Python Variables for Resolver Load

SCRIPT_DIR=/home/workh/Ynput/dev/ayon-usd-resolver/Resolvers/HouLinux/LinuxPy310Houdini20

export USD_ASSET_RESOLVER=$SCRIPT_DIR
export TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT #AYONUSDRESOLVER_RESOLVER_CONTEXT #AYONUSDRESOLVER_RESOLVER
export LD_LIBRARY_PATH=$SCRIPT_DIR/ayonUsdResolver/lib:$LD_LIBRARY_PATH
export PXR_PLUGINPATH_NAME=$SCRIPT_DIR/ayonUsdResolver/resources:$PXR_PLUGINPATH_NAME
export PYTHONPATH=$SCRIPT_DIR/ayonUsdResolver/lib/python:

#export AYON_API_KEY="7cefc33b3e47ef9804034dc6adaca9e92a9ceed03d341bbec46a64b09caaae7d"
#export AYON_SERVER_URL="https://usd.ayon.app"

export AYON_API_KEY="708482aae646caf09279e4699dfa0795a71271dee388cd265cf689cbc8898bde"
export AYON_SERVER_URL="http://192.168.178.42:5000"
#export AYON_SERVER_URL="https://ayontest.open-planck.de"


export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="OFF"
export AYONLOGGERFILEPOS="LoggingFiles"

python ResolverBase/UsdCore.py

