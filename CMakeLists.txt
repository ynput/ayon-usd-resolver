

# this line gets an env varible to add to the install dir so that we can batch build against multiple software packages 
set(SelectedCompilePlugin $ENV{COMPILEPLUGIN} CACHE PATH "install suffix for the resolver")

if (NOT DEFINED ENV{INSTALLNAME})
	set(InstallName ${SelectedCompilePlugin})
else()
	set(InstallName $ENV{INSTALLNAME} CACHE PATH "install suffix for the resolver")
endif()

add_compile_definitions(DEV=${DEV})
add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT) #we need to define this var here because ayon cpp api needs it to enable open sll suport for https 
#(read ayon cpp api main CMakeLists.txt for more info)

set(AR_PROJECT_NAME UsdAssetResolver)
set(AR_RESOLVER_USD_PYTHON_MODULE_NAME usdAssetResolver)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/Resolvers/${InstallName}" CACHE PATH "install dir")

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



# ----------------------- Project Commands 

set(AR_AYONUSDRESOLVER_USD_PLUGIN_NAME ayonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_CXX_CLASS_NAME AyonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME AyonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME ${AR_RESOLVER_USD_PYTHON_MODULE_NAME}.${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME})

set(AR_AYONUSDRESOLVER_TARGET_LIB ayonUsdResolver)
set(AR_AYONUSDRESOLVER_TARGET_PYTHON _${AR_AYONUSDRESOLVER_TARGET_LIB})
set(AR_AYONUSDRESOLVER_INSTALL_PREFIX ${AR_PROJECT_NAME}/${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME})



# shared libary setup commands
if (WIN32)
    set(AR_ARCH_LIB_SUFFIX "dll")
    message(STATUS "AR_ARCH_LIB_SUFFIX dll")
else()
    set(AR_ARCH_LIB_SUFFIX "so")
    message(STATUS "AR_ARCH_LIB_SUFFIX so")
endif()

# Ayon Cpp Api
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api")



if(WIN32)
  set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api/ext/opensslW")
else()
  set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api/ext/opensslL")
endif()
include_directories(${OPENSSL_ROOT_DIR}/include)


# setting default boost namespace 
set(AR_BOOST_NAMESPACE boost)
add_compile_definitions(BOOST_ALL_NO_LIB)


include(BuildPlugins/${SelectedCompilePlugin}.cmake)




# check if the build plugin sets all the neded varibles 
if(NOT DEFINED AR_PXR_INCLUDE_DIR)
    message(FATAL_ERROR "AR_PXR_INCLUDE_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PXR_LIB_DIR)
    message(FATAL_ERROR "AR_PXR_LIB_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PXR_LIB_PREFIX)
    message(FATAL_ERROR "AR_PXR_LIB_PREFIX is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PYTHON_LIB_NUMBER)
    message(FATAL_ERROR "AR_PYTHON_LIB_NUMBER is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PYTHON_LIB_DIR)
    message(FATAL_ERROR "AR_PYTHON_LIB_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PYTHON_INCLUDE_DIR)
    message(FATAL_ERROR "AR_PYTHON_INCLUDE_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_BOOST_INCLUDE_DIR)
    message(FATAL_ERROR "AR_BOOST_INCLUDE_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED BOOST_LIB_DIR)
    message(FATAL_ERROR "BOOST_LIB_DIR is not defined. Please set it before continuing.")
endif()





# Boost
if (WIN32)
    set(AR_BOOST_PYTHON_LIB ${AR_BOOST_NAMESPACE}_${AR_PYTHON_LIB_NUMBER}-mt-x64)
else()
    set(AR_BOOST_PYTHON_LIB ${AR_BOOST_NAMESPACE}_${AR_PYTHON_LIB_NUMBER})
endif()


if (WIN32)
    add_compile_definitions(NOMINMAX)
endif()  


if (WIN32)
    add_compile_options(/Zc:inline- /w)
else()
    add_compile_options(-fPIC  -Wno-deprecated -Wno-deprecated-declarations)
endif()



# --------------------------- Includes / Linking

### Packages ###
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/defaults)
include(setBoostNamespace)
include_directories(${CMAKE_SOURCE_DIR}/src/utils)


# Boost
link_directories(${BOOST_LIB_DIR})
# Usd
link_directories(${AR_PXR_LIB_DIR})
# Python
link_directories(${AR_PYTHON_LIB_DIR})

# Resolver Code
add_subdirectory("src/AyonUsdResolver")

