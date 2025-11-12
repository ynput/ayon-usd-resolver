cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()

project(AyonUsdResolver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- Define Target Names ---
set(CORE_LIB_TARGET AyonUsdResolver)
set(PYTHON_MOD_TARGET AyonUsdResolver_python)

# --- Find Required Packages ---
find_package(USD REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED)
# find_package(Python REQUIRED COMPONENTS Development)
# --- Respect custom Python executable (e.g. Houdini's embedded Python) ---
if(DEFINED PYTHON_EXECUTABLE)
    # Point both modern and legacy modules at Houdini's Python
    set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
    get_filename_component(_python_root "${PYTHON_EXECUTABLE}" DIRECTORY)
    get_filename_component(_python_root "${_python_root}" DIRECTORY)
    set(Python_ROOT_DIR "${_python_root}")
    set(Python3_ROOT_DIR "${_python_root}")
    set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
    message(STATUS "[AYON] Forcing Python executable to: ${PYTHON_EXECUTABLE}")
endif()

# Force use of the new module (CMake 3.12+)
find_package(Python COMPONENTS Interpreter Development REQUIRED)

message(STATUS "[AYON] Found Python include: ${Python_INCLUDE_DIRS}")
message(STATUS "[AYON] Found Python library: ${Python_LIBRARIES}")
message(STATUS "[AYON] Found Python version: ${Python_VERSION}")

# --- Validate USD ---
if(NOT USD_FOUND)
    message(FATAL_ERROR "USD not found. Please ensure -DUSD_ROOT points to a valid USD or Houdini install.")
endif()

# --- Setup include paths (from FindUSD.cmake) ---
# USD_INCLUDE_DIR and USD_LIB_DIR are defined there
set(USD_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}/pxr/external/boost")

# --- Project Source Files ---
set(RESOLVER_SRCS
    resolver.cpp
    resolverContext.cpp
    helpers/resolutionFunctions.cpp
    cache/assetIdentDef.cpp
    cache/resolverContextCache.cpp
    helpers/ayonApiGet.cpp
)

# --- Define the C++ Plugin Library ---
add_library(${CORE_LIB_TARGET} SHARED ${RESOLVER_SRCS})

# --- Define the Python Bindings Module ---
set(PYTHON_MODULE_NAME "ayonUsdResolver")
add_library(${PYTHON_MOD_TARGET} MODULE
    Wrappers_Tokens/wrapResolverContext.cpp
    Wrappers_Tokens/wrapResolver.cpp
    Wrappers_Tokens/wrapResolverTokens.cpp
)

# --- Set Naming Properties ---
set_target_properties(${CORE_LIB_TARGET} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ayonUsdResolver"
)

set_target_properties(${PYTHON_MOD_TARGET} PROPERTIES
    PREFIX "_"
    SUFFIX ".so"
    OUTPUT_NAME "ayonUsdResolver"
)

# --- Include Directories ---
target_include_directories(${CORE_LIB_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Python_INCLUDE_DIRS}
        ${USD_INCLUDE_DIR}
        ${USD_BOOST_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/ext/spdlog/include
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib
        ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include
        ${Boost_INCLUDE_DIRS}
)

target_include_directories(${PYTHON_MOD_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Python_INCLUDE_DIRS}
    ${USD_INCLUDE_DIR}
    ${USD_BOOST_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/ext/spdlog/include
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include
    ${Boost_INCLUDE_DIRS}
)

# --- Link Libraries ---
target_link_libraries(${CORE_LIB_TARGET}
    PRIVATE
        Threads::Threads
        AyonCppApi
        AyonCppDevToolsLib
        ${Python_LIBRARIES}
        USD::ar
        USD::tf
        USD::plug
        USD::arch
)

target_link_libraries(${PYTHON_MOD_TARGET}
    PUBLIC
        ${CORE_LIB_TARGET}
        ${Python_LIBRARIES}
        USD::ar
        USD::tf
        USD::plug
        USD::arch
)

# --- Installation ---
set(PLUGIN_BASE_DIR "ayonUsdResolver")

install(FILES ${CMAKE_SOURCE_DIR}/plugInfo.json
        DESTINATION ${PLUGIN_BASE_DIR}/resources)

install(TARGETS ${CORE_LIB_TARGET}
        DESTINATION ${PLUGIN_BASE_DIR}/lib)

install(TARGETS ${PYTHON_MOD_TARGET}
        DESTINATION ${PLUGIN_BASE_DIR}/lib/python/usdAssetResolver/AyonUsdResolver)
