cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

# this line gets an env variable to add to the install dir so that we can batch build against multiple software packages 
set(SelectedCompilePlugin $ENV{COMPILEPLUGIN} CACHE PATH "install suffix for the resolver")
message(STATUS "Selected Compile Plugin ${SelectedCompilePlugin}")



if (NOT DEFINED ENV{INSTALLNAME})
	set(InstallName ${SelectedCompilePlugin})
else()
	set(InstallName $ENV{INSTALLNAME} CACHE PATH "install suffix for the resolver")
endif()


add_compile_definitions(MACRO_REMOVE_SDF_FORMAT_ARGS=1)

set(AR_PROJECT_NAME UsdAssetResolver)
set(AR_RESOLVER_USD_PYTHON_MODULE_NAME usdAssetResolver)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/Resolvers/${InstallName}" CACHE PATH "install dir")


set(BUILD_SHARED_LIBS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# ----------------------- Project Commands 
set(AR_AYONUSDRESOLVER_USD_PLUGIN_NAME ayonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_CXX_CLASS_NAME AyonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME AyonUsdResolver)
set(AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME ${AR_RESOLVER_USD_PYTHON_MODULE_NAME}.${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME})

set(AR_AYONUSDRESOLVER_TARGET_LIB ayonUsdResolver)
set(AR_AYONUSDRESOLVER_TARGET_PYTHON _${AR_AYONUSDRESOLVER_TARGET_LIB})
set(AR_AYONUSDRESOLVER_INSTALL_PREFIX ${AR_PROJECT_NAME}/${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME})

set(MACRO_REMOVE_SDF_FORMAT_ARGS 1)
#TODO this should not be needed but when we remove it we get segfaults lets find out why
add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT) #we need to define this var here because ayon cpp api needs it to enable open sll support for https 

if(WIN32)
  set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api/ext/opensslW")
else()
  set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api/ext/opensslL")
endif()
include_directories(${OPENSSL_ROOT_DIR}/include)



# shared library setup commands
if (WIN32)
    set(AR_ARCH_LIB_SUFFIX "dll")
    message(STATUS "AR_ARCH_LIB_SUFFIX dll")
else()
    set(AR_ARCH_LIB_SUFFIX "so")
    message(STATUS "AR_ARCH_LIB_SUFFIX so")
endif()

if (WIN32)
    add_compile_definitions(NOMINMAX)
    add_compile_options(/Zc:inline- /w)
else()
    add_compile_options(-fPIC  -Wno-deprecated -Wno-deprecated-declarations)
endif()  


# setting default boost namespace 
set(AR_BOOST_NAMESPACE boost)
add_compile_definitions(BOOST_ALL_NO_LIB)


if (WIN32)
    set(AR_BOOST_EXTEND -mt-x64)
else()
    set(AR_BOOST_EXTEND )
endif()


#-------------- Compile Plugin loading
include(BuildPlugins/${SelectedCompilePlugin}.cmake)
include(cmake/EnsureBuildPluginSafe.cmake)

message(STATUS "SDF_FORMAT_ARGS = ${MACRO_REMOVE_SDF_FORMAT_ARGS}")
add_compile_definitions(MACRO_REMOVE_SDF_FORMAT_ARGS=${MACRO_REMOVE_SDF_FORMAT_ARGS})

# Include AyonDevTools

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-dev-tools")

# Include AyonCppApi 
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api")


# setting CXX11ABI infos
if(DEFINED GLIBCXX_USE_CXX11_ABI)
  message(STATUS "Setting Up CXX11_ABI linking")
  if (NOT WIN32)
      add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=${GLIBCXX_USE_CXX11_ABI})
      target_compile_definitions(AyonCppApi PUBLIC _GLIBCXX_USE_CXX11_ABI=${GLIBCXX_USE_CXX11_ABI})
  endif()
endif()

# Boost
set(AR_BOOST_PYTHON_LIB ${AR_BOOST_NAMESPACE}_${AR_PYTHON_LIB_NUMBER}${AR_BOOST_EXTEND})


project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

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
