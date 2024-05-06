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


# UNREALENGINEENGINEPATH /home/lyonh/Documents/UnrealEngine/Engine 

# set(CMAKE_CXX_COMPILER "clang++")
# plugin dependent settings 
set(AR_UNREAL_ENGINE_ENGINE_FOULDER $ENV{UNREALENGINEENGINEPATH} CACHE PATH "Maya install directory")
set(AR_UNREAL_ENGINE_USDIMPORTER_THIRDPARTY_FOULDER ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Plugins/Importers/USDImporter/Source/ThirdParty )
if (NOT DEFINED ENV{UNREALENGINEENGINEPATH})
  message(FATAL_ERROR "MAYAPATH Env Variable is not defined. But BuildPlugin LinuxPy310Houdini20 needs it to function.") 
endif()


set(AR_PXR_LIB_DIR ${AR_UNREAL_ENGINE_USDIMPORTER_THIRDPARTY_FOULDER}/Linux/bin/x86_64-unknown-linux-gnu)
set(AR_PXR_LIB_PREFIX "usd_") #the library prefix is divergent between Linux and win, some times software developers also rename the prefix
set(AR_PXR_INCLUDE_DIR ${AR_UNREAL_ENGINE_USDIMPORTER_THIRDPARTY_FOULDER}/USD/include)

# TBB include dir
include_directories(${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Intel/TBB/IntelTBB-2019u8/include)

# setting up python 
set(AR_PYTHON_LIB_NUMBER python39)
set(AR_PYTHON_LIB_DIR ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Python3/Linux/include/python3.9)
set(AR_PYTHON_INCLUDE_DIR ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Python3/Linux/include/python3.9)
#set(AR_PYTHON_LIB_SITEPACKAGES ${AR_PYTHON_LIB_DIR}/${AR_PYTHON_LIB}/site-packages)


set(AR_BOOST_EXTEND -mt-x64) 
set(AR_BOOST_INCLUDE_DIR ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty/Boost/boost-1_80_0/include)
set(BOOST_LIB_DIR ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Binaries/Linux)


set(GLIBCXX_USE_CXX11_ABI 1)

set(CMAKE_CXX_COMPILER "${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v22_clang-16.0.6-centos7/x86_64-unknown-linux-gnu/bin/clang++")
set(UE_THIRD_PARTY_LOCATION ${AR_UNREAL_ENGINE_ENGINE_FOULDER}/Source/ThirdParty)
add_compile_options( "-fPIC" "-nostdinc++" "-I${UE_THIRD_PARTY_LOCATION}/Unix/LibCxx/include/c++/v1")
add_link_options("-stdlib=libc++" "-L${UE_THIRD_PARTY_LOCATION}/Unix/LibCxx/lib/Unix/x86_64-unknown-linux-gnu/" "-lc++" "-lc++abi"  "-lm" "-lc" "-lgcc_s" "-lgcc" "-lutil")
