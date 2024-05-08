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


# MAYAUSDPATH /usr/autodesk/mayausd/maya2024/0.25.0_202310160731-bbc8cc8/mayausd
# MAYAUSDDEVKITPATH /path/to/devkit
# MAYAPATH /usr/autodesk/maya2024

# plugin dependent settings 
set(AR_MAYA_ROOT $ENV{MAYAPATH} CACHE PATH "Maya install directory")
if (NOT DEFINED ENV{MAYAPATH})
  message(FATAL_ERROR "MAYAPATH Env Variable is not defined. But BuildPlugin LinuxPy310Houdini20 needs it to function.") 
endif()
set(AR_MAYAUSDPLUGIN_ROOT $ENV{MAYAUSDPATH} CACHE PATH "Maya usd Plugin install directory")
if (NOT DEFINED ENV{MAYAUSDPATH})
  message(FATAL_ERROR "MAYAUSDPATH Env Variable is not defined. But BuildPlugin LinuxPy310Houdini20 needs it to function.") 
endif()
set(AR_MAYAUSDPLUGIN_DEVKIT_ROOT $ENV{MAYAUSDDEVKITPATH} CACHE PATH "Maya usd DevKit Plugin install directory")
if (NOT DEFINED ENV{MAYAUSDDEVKITPATH})
  message(FATAL_ERROR "MAYAUSDDEVKITPATH Env Variable is not defined. But BuildPlugin LinuxPy310Houdini20 needs it to function.") 
endif()


set(AR_PXR_LIB_DIR ${AR_MAYAUSDPLUGIN_DEVKIT_ROOT}/lib)
set(AR_PXR_LIB_PREFIX "usd_") #the library prefix is divergent between Linux and win, some times software developers also rename the prefix
set(AR_PXR_INCLUDE_DIR ${AR_MAYAUSDPLUGIN_DEVKIT_ROOT}/include)


# 'python310.lib'
# setting up python 
set(AR_PYTHON_LIB_NUMBER python310)
set(AR_PYTHON_LIB_DIR ${AR_MAYA_ROOT}/lib)
set(AR_PYTHON_INCLUDE_DIR ${AR_MAYA_ROOT}/include/Python310/Python)
#set(AR_PYTHON_LIB_SITEPACKAGES ${AR_PYTHON_LIB_DIR}/${AR_PYTHON_LIB}/site-packages)

set(AR_BOOST_EXTEND -vc142-mt-x64-1_76)
set(AR_BOOST_INCLUDE_DIR ${AR_MAYAUSDPLUGIN_DEVKIT_ROOT}/include/boost-1_76)
set(BOOST_LIB_DIR ${AR_MAYAUSDPLUGIN_ROOT}/lib)

#set(GLIBCXX_USE_CXX11_ABI 1)