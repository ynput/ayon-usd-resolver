cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()

project(AyonUsdResolver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- Target names ---
set(CORE_LIB_TARGET AyonUsdResolver)
set(PYTHON_MOD_TARGET AyonUsdResolver_python)

# --- Platform specifics ---
if(WIN32)
    set(AR_ARCH_LIB_SUFFIX "dll")
    set(AR_ARCH_PATH_SEP ";")
else()
    set(AR_ARCH_LIB_SUFFIX "so")
    set(AR_ARCH_PATH_SEP ":")
endif()

# =========================================================
# FIND USD & THREADS
# =========================================================
find_package(USD REQUIRED)
find_package(Threads REQUIRED)

# =========================================================
# MAYA DCC SETUP
# =========================================================
if (BUILD_TARGET STREQUAL "maya")
    message(STATUS "[AYON] Configuring for Maya...")
    
    # 1. Maya Core Libraries (OpenMaya, Foundation)
    set(MAYA_LIB_DIR "${MAYA_ROOT}/lib")
    if(NOT EXISTS ${MAYA_LIB_DIR})
        message(FATAL_ERROR "Maya lib directory not found: ${MAYA_LIB_DIR}")
    endif()

    find_library(MAYA_LIB_FOUNDATION Foundation HINTS "${MAYA_LIB_DIR}" REQUIRED)
    find_library(MAYA_LIB_OPENMAYA OpenMaya HINTS "${MAYA_LIB_DIR}" REQUIRED)
    
    # Create imported target for Maya Core
    add_library(Maya::Core UNKNOWN IMPORTED)
    set_target_properties(Maya::Core PROPERTIES
        IMPORTED_LOCATION "${MAYA_LIB_OPENMAYA}"
        INTERFACE_LINK_LIBRARIES "${MAYA_LIB_FOUNDATION}"
        INTERFACE_INCLUDE_DIRECTORIES "${MAYA_ROOT}/include"
        INTERFACE_COMPILE_DEFINITIONS "REQUIRE_IOSTREAM;LINUX;_BOOL"
    )

    # 2. Maya USD Devkit Headers
    if(NOT MAYA_USD_DEVKIT_PATH)
        message(FATAL_ERROR "MAYA_USD_DEVKIT_PATH must be set for Maya build.")
    endif()

    # Create imported target for Maya-USD Devkit headers
    add_library(Maya::USDDevkit INTERFACE IMPORTED)
    set_target_properties(Maya::USDDevkit PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MAYA_USD_DEVKIT_PATH}/include"
    )

    message(STATUS "[AYON] Maya Core found: ${MAYA_LIB_OPENMAYA}")
    message(STATUS "[AYON] Maya USD Devkit: ${MAYA_USD_DEVKIT_PATH}")
endif()

# =========================================================
# Houdini USD / Boost mode
# =========================================================
if(_usd_is_houdini)
    if(EXISTS "${AR_PXR_INCLUDE_DIR}/pxr/external/boost")
        set(AR_BOOST_PXR_EXTERNAL_EXISTS 1)
        set(AR_BOOST_NAMESPACE pxr_boost)
        set(AR_BOOST_FOLDER_NAMESPACE pxr/external/boost)
        if(WIN32)
            set(AR_BOOST_PYTHON_LIB lib${AR_BOOST_NAMESPACE} ${AR_PXR_LIB_PREFIX}python)
        else()
            set(AR_BOOST_PYTHON_LIB ${AR_BOOST_NAMESPACE} ${AR_PXR_LIB_PREFIX}python)
        endif()
        set(AR_BOOST_LIB_DIR ${AR_HOUDINI_LIB_DIR})
        set(AR_BOOST_INCLUDE_DIR ${AR_PXR_INCLUDE_DIR}/pxr/external)
    else()
        add_compile_definitions(HBOOST_ALL_NO_LIB)
        set(AR_BOOST_PXR_EXTERNAL_EXISTS 0)
        set(AR_BOOST_NAMESPACE hboost)
        set(AR_BOOST_FOLDER_NAMESPACE hboost)
        if(WIN32)
            set(AR_BOOST_PYTHON_LIB ${AR_BOOST_NAMESPACE}_${AR_PYTHON_LIB_NUMBER}-mt-x64)
        else()
            set(AR_BOOST_PYTHON_LIB ${AR_BOOST_NAMESPACE}_${AR_PYTHON_LIB_NUMBER})
        endif()
        set(AR_BOOST_LIB_DIR ${AR_HOUDINI_LIB_DIR})
        set(AR_BOOST_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR})
    endif()
endif()

find_package(Boost REQUIRED)

# =========================================================
# PYTHON (Respect Houdini embedded python & Maya Devkit)
# =========================================================
if (BUILD_TARGET STREQUAL "maya")
    # Force CMake to check the Maya Devkit for Python headers
    if (DEFINED MAYA_USD_DEVKIT_PATH)
        message(STATUS "[AYON] Appending Maya Devkit to search path: ${MAYA_USD_DEVKIT_PATH}")
        list(APPEND CMAKE_PREFIX_PATH "${MAYA_USD_DEVKIT_PATH}")
        
        # HELPER: If standard search fails, these hints help CMake find the unusual layout
        set(Python_INCLUDE_DIR "${MAYA_USD_DEVKIT_PATH}/include/Python311/Python")
        list(APPEND CMAKE_INCLUDE_PATH "${MAYA_USD_DEVKIT_PATH}/include/Python311/Python")
    endif()
endif()

if(DEFINED PYTHON_EXECUTABLE)
    set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
    
    # Try to derive root from executable if not in Maya mode
    if (NOT BUILD_TARGET STREQUAL "maya")
        get_filename_component(_python_root "${PYTHON_EXECUTABLE}" DIRECTORY)
        get_filename_component(_python_root "${_python_root}" DIRECTORY)
        set(Python_ROOT_DIR "${_python_root}")
        set(Python3_ROOT_DIR "${_python_root}")
    endif()
    
    message(STATUS "[AYON] Forcing Python executable: ${PYTHON_EXECUTABLE}")
endif()

find_package(Python COMPONENTS Interpreter Development REQUIRED)

message(STATUS "[AYON] Python includes: ${Python_INCLUDE_DIRS}")
message(STATUS "[AYON] Python libs:     ${Python_LIBRARIES}")
message(STATUS "[AYON] Python version:  ${Python_VERSION}")

# =========================================================
# USD sanity
# =========================================================
if(NOT USD_FOUND)
    message(FATAL_ERROR "USD not found. Set -DUSD_ROOT to Houdini or USD installation.")
endif()

set(USD_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}/pxr/external/boost")

# =========================================================
# Sources
# =========================================================
set(RESOLVER_SRCS
    resolver.cpp
    resolverContext.cpp
    helpers/resolutionFunctions.cpp
    cache/assetIdentDef.cpp
    cache/resolverContextCache.cpp
    helpers/ayonApiGet.cpp
)

add_library(${CORE_LIB_TARGET} SHARED ${RESOLVER_SRCS})

add_library(${PYTHON_MOD_TARGET} MODULE
    Wrappers_Tokens/wrapResolverContext.cpp
    Wrappers_Tokens/wrapResolver.cpp
    Wrappers_Tokens/wrapResolverTokens.cpp
)

set_target_properties(${CORE_LIB_TARGET}
    PROPERTIES PREFIX "" OUTPUT_NAME "ayonUsdResolver")

set_target_properties(${PYTHON_MOD_TARGET}
    PROPERTIES PREFIX "_" SUFFIX ".so" OUTPUT_NAME "ayonUsdResolver")

# =========================================================
# FIX: Use ONLY Houdini’s bundled SPDLOG
# =========================================================
set(HOUDINI_SPDLOG_INCLUDE "$ENV{HFS}/toolkit/include")
set(HOUDINI_SPDLOG_LIB_DIR "$ENV{HFS}/dsolib")

# =========================================================
# INCLUDE DIRECTORIES
# =========================================================
target_include_directories(${CORE_LIB_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${USD_INCLUDE_DIR}
        ${USD_BOOST_INCLUDE_DIR}
        ${Python_INCLUDE_DIRS}

        # AYON API
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi

        # Dev-tools
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src/lib

        # DCC-SPECIFIC INCLUDES (Houdini only)
        # Maya includes are handled via target linking below
        $<IF:$<BOOL:${_usd_is_houdini}>,${HOUDINI_SPDLOG_INCLUDE},>

        # Other deps
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include

        ${Boost_INCLUDE_DIRS}
)

target_include_directories(${PYTHON_MOD_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${USD_INCLUDE_DIR}
        ${USD_BOOST_INCLUDE_DIR}
        ${Python_INCLUDE_DIRS}

        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src/lib

        # DCC-SPECIFIC INCLUDES (Houdini only)
        $<IF:$<BOOL:${_usd_is_houdini}>,${HOUDINI_SPDLOG_INCLUDE},>

        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include

        ${Boost_INCLUDE_DIRS}
)

# =========================================================
# LIBRARY LINKING
# =========================================================
# 1. Base Linking (Common)
target_link_libraries(${CORE_LIB_TARGET}
    PRIVATE
        Threads::Threads
        AyonCppApi
        AyonCppDevToolsLib
        ${Python_LIBRARIES}
        
        # Core USD libs
        USD::ar
        USD::tf
        USD::plug
        USD::arch
)

target_link_libraries(${PYTHON_MOD_TARGET}
    PUBLIC
        ${CORE_LIB_TARGET}
        ${Python_LIBRARIES}
        
        # Core USD libs
        USD::ar
        USD::tf
        USD::plug
        USD::arch
)

# 2. Maya Specific Linking
# We do this here inside the IF block so the targets exist.
# Linking "Maya::USDDevkit" automatically adds its INTERFACE_INCLUDE_DIRECTORIES
if (BUILD_TARGET STREQUAL "maya")
    target_link_libraries(${CORE_LIB_TARGET} PRIVATE Maya::Core Maya::USDDevkit)
    target_link_libraries(${PYTHON_MOD_TARGET} PUBLIC Maya::Core Maya::USDDevkit)
endif()

# =========================================================
# INSTALLATION
# =========================================================
set(PLUGIN_BASE_DIR "ayonUsdResolver")

install(FILES ${CMAKE_SOURCE_DIR}/plugInfo.json
        DESTINATION ${PLUGIN_BASE_DIR}/resources)

install(TARGETS ${CORE_LIB_TARGET}
        DESTINATION ${PLUGIN_BASE_DIR}/lib)

install(TARGETS ${PYTHON_MOD_TARGET}
        DESTINATION ${PLUGIN_BASE_DIR}/lib/python/usdAssetResolver/AyonUsdResolver)
        