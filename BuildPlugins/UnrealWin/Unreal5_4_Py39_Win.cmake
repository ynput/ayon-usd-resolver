# LinuxPy310Houdini20

# Variables that need to be set by the plugin (the env variables will be checked by the main cmakeLists script and an error will accrue if one ore more are missing)
# AR_PXR_INCLUDE_DIR
# AR_PXR_LIB_DIR
# AR_PYTHON_LIB_NUMBER
# AR_BOOST_INCLUDE_DIR
# BOOST_LIB_DIR
# AR_PYTHON_LIB_DIR
# AR_PYTHON_INCLUDE_DIR
# AR_PXR_LIB_PREFIX


# UNREALENGINEENGINEPATH C:/Program Files/Epic Games/UE_5.4/Engine 

# plugin dependent settings
if (NOT DEFINED ENV{UNREALENGINEENGINEPATH})
  message(FATAL_ERROR "UNREALENGINEENGINEPATH Env Variable is not defined. But BuildPlugin needs it to function.") 
endif()

set(AR_UNREAL_ENGINE_ENGINE_FOULDER $ENV{UNREALENGINEENGINEPATH} CACHE PATH "Unreal install directory")
set(UE_THIRD_PARTY_LOCATION ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty)
set(AR_UNREAL_ENGINE_USDIMPORTER_THIRDPARTY_FOULDER ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Plugins/Importers/USDImporter/Source/ThirdParty )


set(AR_PXR_LIB_DIR ${AR_UNREAL_ENGINE_USDIMPORTER_THIRDPARTY_FOULDER}/USD/lib)
set(AR_PXR_LIB_PREFIX "usd_") #the library prefix is divergent between Linux and win, some times software developers also rename the prefix
set(AR_PXR_INCLUDE_DIR ${AR_UNREAL_ENGINE_USDIMPORTER_THIRDPARTY_FOULDER}/USD/include)


include_directories(${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Intel/TBB/IntelTBB-2019u8/include)
link_directories(${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Intel/TBB/IntelTBB-2019u8/lib/Win64/vc14)


# setting up python 
set(AR_PYTHON_LIB_NUMBER python311)
set(AR_PYTHON_LIB_DIR ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Python3/Win64/libs)
set(AR_PYTHON_INCLUDE_DIR ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Python3/Win64/include)
#set(AR_PYTHON_LIB_SITEPACKAGES ${AR_PYTHON_LIB_DIR}/${AR_PYTHON_LIB}/site-packages)

set(AR_BOOST_EXTEND -mt-x64) 
set(AR_BOOST_INCLUDE_DIR ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Boost/boost-1_82_0/include)
set(BOOST_LIB_DIR ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Boost/boost-1_82_0/lib/win64)