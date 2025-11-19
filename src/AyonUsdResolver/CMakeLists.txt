# cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
# if(POLICY CMP0148)
#     cmake_policy(SET CMP0148 NEW)
# endif()

# project(AyonUsdResolver)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# # --- Define Target Names ---
# set(CORE_LIB_TARGET AyonUsdResolver)
# set(PYTHON_MOD_TARGET AyonUsdResolver_python)

# # --- Find Required Packages ---
# find_package(USD REQUIRED)
# find_package(Threads REQUIRED)
# find_package(Boost REQUIRED)

# # ---- Debug Output for Boost ----
# message(STATUS "Boost_FOUND: ${Boost_FOUND}")
# message(STATUS "Boost_VERSION: ${Boost_VERSION}")
# message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
# message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
# message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")

# # find_package(Python REQUIRED COMPONENTS Development)
# # --- Respect custom Python executable (e.g. Houdini's embedded Python) ---
# if(DEFINED PYTHON_EXECUTABLE)
#     # Point both modern and legacy modules at Houdini's Python
#     set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
#     get_filename_component(_python_root "${PYTHON_EXECUTABLE}" DIRECTORY)
#     get_filename_component(_python_root "${_python_root}" DIRECTORY)
#     set(Python_ROOT_DIR "${_python_root}")
#     set(Python3_ROOT_DIR "${_python_root}")
#     set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
#     message(STATUS "[AYON] Forcing Python executable to: ${PYTHON_EXECUTABLE}")
# endif()

# # Force use of the new module (CMake 3.12+)
# find_package(Python COMPONENTS Interpreter Development REQUIRED)

# message(STATUS "[AYON] Found Python include: ${Python_INCLUDE_DIRS}")
# message(STATUS "[AYON] Found Python library: ${Python_LIBRARIES}")
# message(STATUS "[AYON] Found Python version: ${Python_VERSION}")

# # --- Validate USD ---
# if(NOT USD_FOUND)
#     message(FATAL_ERROR "USD not found. Please ensure -DUSD_ROOT points to a valid USD or Houdini install.")
# endif()

# # --- Setup include paths (from FindUSD.cmake) ---
# # USD_INCLUDE_DIR and USD_LIB_DIR are defined there
# set(USD_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}/pxr/external/boost")

# # --- Project Source Files ---
# set(RESOLVER_SRCS
#     resolver.cpp
#     resolverContext.cpp
#     helpers/resolutionFunctions.cpp
#     cache/assetIdentDef.cpp
#     cache/resolverContextCache.cpp
#     helpers/ayonApiGet.cpp
# )

# # --- Define the C++ Plugin Library ---
# add_library(${CORE_LIB_TARGET} SHARED ${RESOLVER_SRCS})

# # --- Define the Python Bindings Module ---
# set(PYTHON_MODULE_NAME "ayonUsdResolver")
# add_library(${PYTHON_MOD_TARGET} MODULE
#     Wrappers_Tokens/wrapResolverContext.cpp
#     Wrappers_Tokens/wrapResolver.cpp
#     Wrappers_Tokens/wrapResolverTokens.cpp
# )

# # --- Set Naming Properties ---
# set_target_properties(${CORE_LIB_TARGET} PROPERTIES
#     PREFIX ""
#     OUTPUT_NAME "ayonUsdResolver"
# )

# set_target_properties(${PYTHON_MOD_TARGET} PROPERTIES
#     PREFIX "_"
#     SUFFIX ".so"
#     OUTPUT_NAME "ayonUsdResolver"
# )

# # --- Include Directories ---
# target_include_directories(${CORE_LIB_TARGET}
#     PUBLIC
#         ${CMAKE_CURRENT_SOURCE_DIR}
#         ${Python_INCLUDE_DIRS}
#         ${USD_INCLUDE_DIR}
#         ${USD_BOOST_INCLUDE_DIR}
#         ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi
#         ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src
#         ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/ext/spdlog/include
#         ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib
#         ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include
#         ${Boost_INCLUDE_DIRS}
# )

# target_include_directories(${PYTHON_MOD_TARGET} PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${Python_INCLUDE_DIRS}
#     ${USD_INCLUDE_DIR}
#     ${USD_BOOST_INCLUDE_DIR}
#     ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi
#     ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src
#     ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/ext/spdlog/include
#     ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib
#     ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include
#     ${Boost_INCLUDE_DIRS}
# )

# # --- Link Libraries ---
# target_link_libraries(${CORE_LIB_TARGET}
#     PRIVATE
#         Threads::Threads
#         AyonCppApi
#         AyonCppDevToolsLib
#         ${Python_LIBRARIES}
#         USD::ar
#         USD::tf
#         USD::plug
#         USD::arch
# )

# target_link_libraries(${PYTHON_MOD_TARGET}
#     PUBLIC
#         ${CORE_LIB_TARGET}
#         ${Python_LIBRARIES}
#         USD::ar
#         USD::tf
#         USD::plug
#         USD::arch
# )

# # --- Installation ---
# set(PLUGIN_BASE_DIR "ayonUsdResolver")

# install(FILES ${CMAKE_SOURCE_DIR}/plugInfo.json
#         DESTINATION ${PLUGIN_BASE_DIR}/resources)

# install(TARGETS ${CORE_LIB_TARGET}
#         DESTINATION ${PLUGIN_BASE_DIR}/lib)
# install(TARGETS ${PYTHON_MOD_TARGET}
#         DESTINATION ${PLUGIN_BASE_DIR}/lib/python/usdAssetResolver/AyonUsdResolver)

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

# ------------------------------------------------------------------------------
# Build Configuration Switch
# ------------------------------------------------------------------------------
# Options: "Generic", "Houdini"
# Default is "Generic" (Standard USD + Standard Boost)
set(BUILD_TARGET "Generic" CACHE STRING "Target DCC Environment")
message(STATUS "[AYON] Build Target: ${BUILD_TARGET}")

find_package(Threads REQUIRED)

# ------------------------------------------------------------------------------
# Dependency Finding Logic
# ------------------------------------------------------------------------------

if(BUILD_TARGET STREQUAL "Houdini")
    # --- HOUDINI CONFIGURATION ---
    find_package(Houdini REQUIRED)
    
    # Set compiler flags for Houdini environment
    add_compile_definitions(AR_IS_HOUDINI=1)
    add_compile_definitions(FBX_ENABLED=1)
    add_compile_definitions(OPENCL_ENABLED=1)
    add_compile_definitions(OPENVDB_ENABLED=1)

    # Houdini uses 'hboost' namespace
    set(BOOST_NAMESPACE_VAL "hboost")
    
    # Houdini targets include USD, TBB, python, and hboost
    set(USD_LIBS_TO_LINK Houdini)
    
    # Set paths for includes based on Houdini vars
    set(USD_INCLUDE_DIR ${Houdini_INCLUDE_DIRS})
    set(USD_BOOST_INCLUDE_DIR "${Houdini_Toolkit_DIR}/include/pxr/external/boost")
    
    # Use Houdini's Python variables
    set(Python_INCLUDE_DIRS ${Houdini_Python_INCLUDE_DIRS})
    set(Python_LIBRARIES ${Houdini_Python_LIBRARIES})

else()
    # --- GENERIC / STANDALONE CONFIGURATION ---
    add_compile_definitions(AR_IS_GENERIC=1)

    # 1. Find Standard USD
    find_package(USD REQUIRED)
    
    # 2. Find Standard Boost
    find_package(Boost REQUIRED)
    
    # Debug Boost
    message(STATUS "Boost_FOUND: ${Boost_FOUND}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    
    # 3. Find Python (Logic from your file)
    if(DEFINED PYTHON_EXECUTABLE)
        set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
        get_filename_component(_python_root "${PYTHON_EXECUTABLE}" DIRECTORY)
        get_filename_component(_python_root "${_python_root}" DIRECTORY)
        set(Python_ROOT_DIR "${_python_root}")
        set(Python3_ROOT_DIR "${_python_root}")
        set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
        message(STATUS "[AYON] Forcing Python executable to: ${PYTHON_EXECUTABLE}")
    endif()

    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    
    message(STATUS "[AYON] Found Python include: ${Python_INCLUDE_DIRS}")
    message(STATUS "[AYON] Found Python library: ${Python_LIBRARIES}")
    
    # Standard namespace
    set(BOOST_NAMESPACE_VAL "boost")
    
    # Standard USD targets (Assuming your FindUSD.cmake creates these imported targets)
    set(USD_LIBS_TO_LINK 
        USD::ar
        USD::tf
        USD::plug
        USD::arch
        ${Python_LIBRARIES}
    )
    
    # Path setup
    set(USD_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}/pxr/external/boost")
endif()

# --- Global Definitions ---
# Pass the namespace down to C++ so boost_include_wrapper.h works
add_compile_definitions(AR_BOOST_NAMESPACE=${BOOST_NAMESPACE_VAL})


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
# Consolidated list to avoid duplication
set(COMMON_INCLUDES
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
    
    # Added for Python Bindings to find headers installed by Core
    ${CMAKE_INSTALL_PREFIX}/include 
)

target_include_directories(${CORE_LIB_TARGET} PUBLIC ${COMMON_INCLUDES})
target_include_directories(${PYTHON_MOD_TARGET} PUBLIC ${COMMON_INCLUDES})

# --- Link Libraries ---
target_link_libraries(${CORE_LIB_TARGET}
    PRIVATE
        Threads::Threads
        AyonCppApi
        AyonCppDevToolsLib
        ${USD_LIBS_TO_LINK} # Switches based on BUILD_TARGET
)

target_link_libraries(${PYTHON_MOD_TARGET}
    PUBLIC
        ${CORE_LIB_TARGET}
        ${USD_LIBS_TO_LINK} # Switches based on BUILD_TARGET
)

# --- Installation ---
set(PLUGIN_BASE_DIR "ayonUsdResolver")

install(FILES ${CMAKE_SOURCE_DIR}/plugInfo.json
        DESTINATION ${PLUGIN_BASE_DIR}/resources)

install(TARGETS ${CORE_LIB_TARGET}
        DESTINATION ${PLUGIN_BASE_DIR}/lib)
        
install(TARGETS ${PYTHON_MOD_TARGET}
        DESTINATION ${PLUGIN_BASE_DIR}/lib/python/usdAssetResolver/AyonUsdResolver)