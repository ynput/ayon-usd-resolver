# manny off those settings are based on Luca Shellers Resolver // also manny off those settings are subject to change 
# https://github.com/LucaScheller/VFX-UsdAssetResolver/blob/main/CMakeLists.txt


#--------------------------- Define Varibles to be used in the Script for Preprocessor statments 
add_compile_definitions(DEV=${DEV})
set(CMAKE_CXX_STANDARD 20)


# ---------------------- Project



set(AR_PROJECT_NAME UsdAssetResolver)
set(AR_RESOLVER_USD_PYTHON_MODULE_NAME usdAssetResolver)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/dist" CACHE PATH "Default install dir")

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(BUILD_SHARED_LIBS ON)



# ---------------------- Developer Convenience Commands 


# this command exports a file to the build fouler that nvim clangd can use to understand the project dependencies. 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)




# ----------------------- Project Commands 

set(AR_AYONUSDRESOLVER_USD_PLUGIN_NAME ayonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_CXX_CLASS_NAME AyonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME AyonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME ${AR_RESOLVER_USD_PYTHON_MODULE_NAME}.${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME})

# set(AR_AYONUSDRESOLVER_USD_PYTHON_EXPOSE_MODULE_NAME PythonExpose)

set(AR_AYONUSDRESOLVER_TARGET_LIB ayonUsdResolver)
set(AR_AYONUSDRESOLVER_TARGET_PYTHON _${AR_AYONUSDRESOLVER_TARGET_LIB})
set(AR_AYONUSDRESOLVER_INSTALL_PREFIX ${AR_PROJECT_NAME}/${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME})



# Arch
if (WIN32)
    set(AR_ARCH_LIB_SUFFIX "dll")
    message(STATUS "AR_ARCH_LIB_SUFFIX dll")
else()
    set(AR_ARCH_LIB_SUFFIX "so")
    message(STATUS "AR_ARCH_LIB_SUFFIX so")
endif()

# Compiler Options
# This is the same as set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHBOOST_ALL_NO_LIB -D_GLIBCXX_USE_CXX11_ABI=0")
if (WIN32)
    add_compile_options(/Zc:inline- /w)
else()
    add_compile_options(-fPIC  )
endif()






# ---------------------------- Platform commands 



set(AR_HOUDINI_ROOT $ENV{HFS} CACHE PATH "Houdini install directory")




#------------------------- Ayon Cpp Api 


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api")



# Preprocessor Defines (Same as #define)


add_compile_definitions(BOOST_ALL_NO_LIB)

if (WIN32)
    add_compile_definitions(NOMINMAX)
else()
    # Notes:
    # Houdini 20 - Switched to the new C++11 ABI for Linux https://www.sidefx.com/docs/houdini/news/20/platforms.html
    # For Houdini versions that use gcc 9.3, please set this to _GLIBCXX_USE_CXX11_ABI=0
    file(REAL_PATH ${AR_HOUDINI_ROOT} AR_HOUDINI_ROOT_RESOLVED)
    string(FIND ${AR_HOUDINI_ROOT_RESOLVED} "19.5" AR_HOUDINI_ROOT_IS_H195)
    if (${AR_HOUDINI_ROOT_IS_H195} STREQUAL "-1")
        add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)
    else()
        message("Houdini 19.5 detected, switching to _GLIBCXX_USE_CXX11_ABI=0")
        add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
    endif()
endif()








# Houdini


add_compile_definitions(HBOOST_ALL_NO_LIB)
if (WIN32)
    set(AR_HOUDINI_LIB_DIR ${AR_HOUDINI_ROOT}/custom/houdini/dsolib)
else()
    set(AR_HOUDINI_LIB_DIR ${AR_HOUDINI_ROOT}/dsolib)
endif()
set(AR_HOUDINI_INCLUDE_DIR ${AR_HOUDINI_ROOT}/toolkit/include)
# Usd
if (WIN32)
    set(AR_PXR_LIB_DIR ${AR_HOUDINI_ROOT}/custom/houdini/dsolib)
    set(AR_PXR_LIB_PREFIX "libpxr_")
else()
    set(AR_PXR_LIB_DIR ${AR_HOUDINI_ROOT}/dsolib)
    set(AR_PXR_LIB_PREFIX "pxr_")
endif()
set(AR_PXR_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR})

# Python
if (WIN32)
    if(EXISTS "${AR_HOUDINI_ROOT}/python310")
        set(AR_PYTHON_LIB python3.10)
        set(AR_PYTHON_LIB_NUMBER python310)
    else()
        set(AR_PYTHON_LIB python3.9)
        set(AR_PYTHON_LIB_NUMBER python39)
    endif()
else()
    if(EXISTS "${AR_HOUDINI_ROOT}/python/bin/python3.10")
        set(AR_PYTHON_LIB python3.10)
        set(AR_PYTHON_LIB_NUMBER python310)
    else()
        set(AR_PYTHON_LIB python3.9)
        set(AR_PYTHON_LIB_NUMBER python39)
    endif()
endif()

if (WIN32)
    set(AR_PYTHON_LIB_DIR ${AR_HOUDINI_ROOT}/${AR_PYTHON_LIB_NUMBER}/libs)
    set(AR_PYTHON_LIB_SITEPACKAGES ${AR_HOUDINI_ROOT}/${AR_PYTHON_LIB_NUMBER}/lib/site-packages)
else()
    set(AR_PYTHON_LIB_DIR ${AR_HOUDINI_ROOT}/python/lib)
    set(AR_PYTHON_LIB_SITEPACKAGES ${AR_PYTHON_LIB_DIR}/${AR_PYTHON_LIB}/site-packages)
endif()
set(AR_PYTHON_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR}/${AR_PYTHON_LIB})

set(AR_BOOST_NAMESPACE hboost)

# Boost
if (WIN32)
    set(AR_BOOST_PYTHON_LIB ${AR_BOOST_NAMESPACE}_${AR_PYTHON_LIB_NUMBER}-mt-x64)
else()
    set(AR_BOOST_PYTHON_LIB ${AR_BOOST_NAMESPACE}_${AR_PYTHON_LIB_NUMBER})
endif()
set(AR_BOOST_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR}/${AR_BOOST_NAMESPACE})














# --------------------------- Includes / Linking



### Packages ###
# Defaults
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/defaults)
# Boost Namespace Wrapper
include(setBoostNamespace)
include_directories(${CMAKE_SOURCE_DIR}/src/utils)
# Houdini
link_directories(${AR_HOUDINI_LIB_DIR})
# Usd (Already provided via Houdini)
link_directories(${AR_PXR_LIB_DIR})
# Python
link_directories(${AR_PYTHON_LIB_DIR})

### Targets ###
## Maintained in this repo
add_subdirectory("src/AyonUsdResolver")






# ------------------------ Install




