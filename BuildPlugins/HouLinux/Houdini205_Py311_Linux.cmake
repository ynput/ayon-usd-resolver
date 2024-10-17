# LinuxPy310Houdini20

# Variables that need to be set by the plugin (the env variables will be checked by the main cmakeLists script and an error will accrue if one or more are missing)
# AR_PXR_INCLUDE_DIR
# AR_PXR_LIB_DIR
# AR_PYTHON_LIB_NUMBER
# AR_BOOST_INCLUDE_DIR
# BOOST_LIB_DIR
# AR_PYTHON_LIB_DIR
# AR_PYTHON_INCLUDE_DIR
# AR_PXR_LIB_PREFIX


# plugin dependent settings 
set(AR_HOUDINI_ROOT $ENV{HFS} CACHE PATH "Houdini install directory")
if (NOT DEFINED ENV{HFS})
    message(FATAL_ERROR "HFS Env Variable is not defined. But BuildPlugin LinuxPy311Houdini20 needs it to function.") 
endif()

set(AR_HOUDINI_LIB_DIR ${AR_HOUDINI_ROOT}/dsolib)
set(AR_HOUDINI_INCLUDE_DIR ${AR_HOUDINI_ROOT}/toolkit/include)

set(AR_PXR_LIB_DIR ${AR_HOUDINI_ROOT}/dsolib)
set(AR_PXR_LIB_PREFIX "pxr_") #the library prefix is divergent between Linux and win, some times software developers also rename the prefix
set(AR_PXR_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR})

# setting up python 
set(AR_PYTHON_LIB_NUMBER python311)
set(AR_PYTHON_LIB_DIR ${AR_HOUDINI_ROOT}/python/lib)
set(AR_PYTHON_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR}/python3.11)
#set(AR_PYTHON_LIB_SITEPACKAGES ${AR_PYTHON_LIB_DIR}/${AR_PYTHON_LIB}/site-packages)

# setting up boost 
add_compile_definitions(HBOOST_ALL_NO_LIB)
set(AR_BOOST_NAMESPACE hboost) # overwriting boost name space because Houdini renamed it to hboost internally
set(AR_BOOST_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR}/${AR_BOOST_NAMESPACE})
set(BOOST_LIB_DIR ${AR_HOUDINI_INCLUDE_DIR})

# setting Cxx11Abi to on because Hou20 needs it to function 

set(GLIBCXX_USE_CXX11_ABI 1)

# Houdini include dir (might shadow some other library but that's what we want)
link_directories(${AR_HOUDINI_LIB_DIR})
