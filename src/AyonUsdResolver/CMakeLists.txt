cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()

project(AyonUsdResolver)

# =========================================================
# GLOBAL COMPILER FLAGS (CRITICAL ABI FIXES)
# =========================================================
# We apply these globally to ensure NO part of the code misses the Houdini namespace.
message(STATUS "[FORCE] Applying Houdini Namespace and Iterator fixes globally.")

# 1. Force plugin to use Houdini's internal USD namespace (pxrInternal_v0_25...)
#    This fixes the "Failed to manufacture asset resolver" error.
add_compile_definitions(PXR_SET_INTERNAL_NAMESPACE)

# 2. Prevent linking against boost libraries (Houdini provides them)
add_compile_definitions(HBOOST_ALL_NO_LIB)

# 3. Windows-specific Safety Flags
if(MSVC)
    # Match Houdini's Release Mode Iterator Debug Level (Crucial for ABI compatibility)
    add_compile_definitions("_ITERATOR_DEBUG_LEVEL=0")
    
    # TBB Flags to match Houdini
    add_compile_definitions("TBB_USE_ASSERT=0")
    add_compile_definitions("TBB_USE_THREADING_TOOLS=0")
    add_compile_definitions("__TBB_NO_IMPLICIT_LINKAGE")
endif()

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
# OPENSSL CONFIGURATION
# =========================================================
if(WIN32)
    set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/opensslW")
else()
    set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/openssl1L")
endif()

find_package(OpenSSL REQUIRED)

# =========================================================
# MAYA DCC SETUP
# =========================================================
if (BUILD_TARGET STREQUAL "maya")
    message(STATUS "[AYON] Configuring for Maya...")
    set(MAYA_LIB_DIR "${MAYA_ROOT}/lib")
    if(NOT EXISTS ${MAYA_LIB_DIR})
        message(FATAL_ERROR "Maya lib directory not found: ${MAYA_LIB_DIR}")
    endif()
    find_library(MAYA_LIB_FOUNDATION Foundation HINTS "${MAYA_LIB_DIR}" REQUIRED)
    find_library(MAYA_LIB_OPENMAYA OpenMaya HINTS "${MAYA_LIB_DIR}" REQUIRED)
    add_library(Maya::Core UNKNOWN IMPORTED)
    set_target_properties(Maya::Core PROPERTIES
        IMPORTED_LOCATION "${MAYA_LIB_OPENMAYA}"
        INTERFACE_LINK_LIBRARIES "${MAYA_LIB_FOUNDATION}"
        INTERFACE_INCLUDE_DIRECTORIES "${MAYA_ROOT}/include"
        INTERFACE_COMPILE_DEFINITIONS "REQUIRE_IOSTREAM;_BOOL"
    )
    if(NOT WIN32)
        set_property(TARGET Maya::Core APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS "LINUX")
    endif()
    if(NOT MAYA_USD_DEVKIT_PATH)
        message(FATAL_ERROR "MAYA_USD_DEVKIT_PATH must be set for Maya build.")
    endif()
    add_library(Maya::USDDevkit INTERFACE IMPORTED)
    set_target_properties(Maya::USDDevkit PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MAYA_USD_DEVKIT_PATH}/include"
    )
    if(NOT WIN32)
        set(MAYA_PY_INC "${MAYA_ROOT}/include/Python311/Python")
        set(MAYA_PY_LIB "${MAYA_LIB_DIR}/libpython3.11.so")
        if(EXISTS "${MAYA_PY_INC}" AND EXISTS "${MAYA_PY_LIB}")
            set(Python_INCLUDE_DIRS "${MAYA_PY_INC}")
            set(Python_LIBRARIES    "${MAYA_PY_LIB}")
            set(Python_VERSION      "3.11")
            set(Python_FOUND TRUE)
            set(Python_Interpreter_FOUND TRUE)
            set(Python_Development_FOUND TRUE)
        else()
             set(MAYA_PY_INC "${MAYA_USD_DEVKIT_PATH}/include/Python311/Python")
             if(EXISTS "${MAYA_PY_INC}")
                set(Python_INCLUDE_DIRS "${MAYA_PY_INC}")
                set(Python_LIBRARIES    "${MAYA_PY_LIB}")
                set(Python_FOUND TRUE)
             endif()
        endif()
    endif()
    if(WIN32)
        set(MAYA_PYTHON_INCLUDE_HINT "${MAYA_USD_DEVKIT_PATH}/include/Python311/Python")
        list(APPEND CMAKE_INCLUDE_PATH "${MAYA_PYTHON_INCLUDE_HINT}")
        set(Python_INCLUDE_DIR "${MAYA_PYTHON_INCLUDE_HINT}")
    endif()
    list(APPEND CMAKE_PREFIX_PATH "${MAYA_USD_DEVKIT_PATH}")
endif()

# =========================================================
# Houdini USD / Boost Setup
# =========================================================
if(_usd_is_houdini)
    message(STATUS "[AYON] Configuring libraries for Houdini...")
    
    # Note: Global flags (PXR_SET_INTERNAL_NAMESPACE, etc.) are already set at the top.

    # [FIX] Find libpxr_python.lib to prevent LNK2019 errors
    if(WIN32)
        set(HOUDINI_PXR_PYTHON_LIB "${USD_ROOT}/custom/houdini/dsolib/libpxr_python.lib")
        if(EXISTS ${HOUDINI_PXR_PYTHON_LIB})
            set(USD_PYTHON_LIBRARIES ${HOUDINI_PXR_PYTHON_LIB})
            message(STATUS "[AYON] Found libpxr_python: ${HOUDINI_PXR_PYTHON_LIB}")
        else()
            message(WARNING "[AYON] Could not find libpxr_python.lib! Linking may fail.")
        endif()
        set(HOUDINI_SPDLOG_INCLUDE "${USD_ROOT}/toolkit/include")
    endif()

    if(EXISTS "${AR_PXR_INCLUDE_DIR}/pxr/external/boost")
        set(AR_BOOST_NAMESPACE pxr_boost)
        set(AR_BOOST_INCLUDE_DIR ${AR_PXR_INCLUDE_DIR}/pxr/external)
    else()
        set(AR_BOOST_NAMESPACE hboost)
        set(AR_BOOST_INCLUDE_DIR ${USD_INCLUDE_DIR}) 
    endif()

    set(Boost_FOUND TRUE)
    set(Boost_INCLUDE_DIRS ${AR_BOOST_INCLUDE_DIR})
else()
    find_package(Boost REQUIRED)
endif()

# =========================================================
# PYTHON
# =========================================================
if(NOT Python_FOUND)
    if(DEFINED PYTHON_EXECUTABLE)
        set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
        if (NOT BUILD_TARGET STREQUAL "maya")
            get_filename_component(_python_root "${PYTHON_EXECUTABLE}" DIRECTORY)
            get_filename_component(_python_root "${_python_root}" DIRECTORY)
            set(Python_ROOT_DIR "${_python_root}")
        endif()
    endif()
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
endif()

# =========================================================
# INCLUDE DIRECTORIES
# =========================================================
set(RESOLVER_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${USD_INCLUDE_DIR}
    ${USD_BOOST_INCLUDE_DIR}
    ${Python_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src/lib
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/spdlog/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)
if(_usd_is_houdini AND HOUDINI_SPDLOG_INCLUDE)
    list(APPEND RESOLVER_INCLUDE_DIRS ${HOUDINI_SPDLOG_INCLUDE})
endif()

# =========================================================
# Target: Core Library
# =========================================================
set(RESOLVER_SRCS
    resolver.cpp
    resolverContext.cpp
    helpers/resolutionFunctions.cpp
    cache/assetIdentifierDef.cpp
    cache/resolverContextCache.cpp
    helpers/ayonApiGet.cpp
)

add_library(${CORE_LIB_TARGET} SHARED ${RESOLVER_SRCS})

if(WIN32)
    target_compile_definitions(${CORE_LIB_TARGET} PRIVATE AR_AYONUSDRESOLVER_EXPORTS)
endif()

target_include_directories(${CORE_LIB_TARGET} PUBLIC ${RESOLVER_INCLUDE_DIRS})
set_target_properties(${CORE_LIB_TARGET} PROPERTIES PREFIX "" OUTPUT_NAME "ayonUsdResolver")

# =========================================================
# [FIX] MSVC: prevent USD registry/constructor stripping in plugin DLL
# =========================================================
if (MSVC)
    target_compile_options(${CORE_LIB_TARGET} PRIVATE /Zc:inline-)
    target_link_options(${CORE_LIB_TARGET} PRIVATE /OPT:NOREF)

    # Optional extra safety (uncomment if still stripped)
    # target_link_options(${CORE_LIB_TARGET} PRIVATE /OPT:NOICF)
endif()

# =========================================================
# Target: Python Module
# =========================================================
add_library(${PYTHON_MOD_TARGET} MODULE
    Wrappers_Tokens/wrapResolverContext.cpp
    Wrappers_Tokens/wrapResolver.cpp
    Wrappers_Tokens/wrapResolverTokens.cpp
    pluginData/module.cpp 
)

target_include_directories(${PYTHON_MOD_TARGET} PRIVATE ${RESOLVER_INCLUDE_DIRS})
target_compile_definitions(${PYTHON_MOD_TARGET} PRIVATE AR_AYONUSDRESOLVER_EXPORTS)

# (Optional) Apply same MSVC options to python module too, just in case
if (MSVC)
    target_compile_options(${PYTHON_MOD_TARGET} PRIVATE /Zc:inline-)
    target_link_options(${PYTHON_MOD_TARGET} PRIVATE /OPT:NOREF)
endif()

# =========================================================
# LIBRARY LINKING
# =========================================================
target_link_libraries(${CORE_LIB_TARGET}
    PRIVATE
        Threads::Threads
        AyonCppApi
        AyonCppDevToolsLib
        ${Python_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        USD::ar
        USD::tf
        USD::plug
        USD::arch
        ${USD_PYTHON_LIBRARIES}
)

target_link_libraries(${PYTHON_MOD_TARGET}
    PUBLIC
        ${CORE_LIB_TARGET}
        ${Python_LIBRARIES}
        USD::ar
        USD::tf
        USD::plug
        USD::arch
        ${USD_PYTHON_LIBRARIES}
)

if (BUILD_TARGET STREQUAL "maya")
    target_link_libraries(${CORE_LIB_TARGET} PRIVATE Maya::Core Maya::USDDevkit)
    target_link_libraries(${PYTHON_MOD_TARGET} PUBLIC Maya::Core Maya::USDDevkit)
endif()

# =========================================================
# INSTALLATION
# =========================================================
set(PLUGIN_BASE_DIR "ayonUsdResolver")

set(AR_AYONUSDRESOLVER_USD_CXX_CLASS_NAME "AyonUsdResolver")
set(AR_AYONUSDRESOLVER_USD_PLUGIN_NAME "ayonUsdResolver")
set(AR_AYONUSDRESOLVER_TARGET_LIB "ayonUsdResolver") 

if(WIN32)
    set(AR_ARCH_LIB_SUFFIX "dll")
    set_target_properties(${PYTHON_MOD_TARGET} PROPERTIES PREFIX "" SUFFIX ".pyd" OUTPUT_NAME "_ayonUsdResolver") 
else()
    set(AR_ARCH_LIB_SUFFIX "so")
    set_target_properties(${PYTHON_MOD_TARGET} PROPERTIES PREFIX "" SUFFIX ".so" OUTPUT_NAME "_ayonUsdResolver")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json.in
    ${CMAKE_BINARY_DIR}/plugInfo.json
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/plugInfo.json DESTINATION ${PLUGIN_BASE_DIR}/resources)
install(TARGETS ${CORE_LIB_TARGET} DESTINATION ${PLUGIN_BASE_DIR}/lib)
install(TARGETS ${PYTHON_MOD_TARGET} DESTINATION ${PLUGIN_BASE_DIR}/lib/python/usdAssetResolver/AyonUsdResolver)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py DESTINATION ${PLUGIN_BASE_DIR}/lib/python/usdAssetResolver/AyonUsdResolver)
endif()

# =========================================================
# INSTALL OPENSSL DLLs (Windows Only)
# =========================================================
if(WIN32)
    message(STATUS "[AYON] Configuring install for OpenSSL DLLs from: ${OPENSSL_ROOT_DIR}")
    
    # We install the specific DLLs required for OpenSSL 3
    # We check both the root and /bin in case you copied them to a subfolder
    if(EXISTS "${OPENSSL_ROOT_DIR}/libcrypto-3-x64.dll")
        install(FILES 
            "${OPENSSL_ROOT_DIR}/libcrypto-3-x64.dll"
            "${OPENSSL_ROOT_DIR}/libssl-3-x64.dll"
            DESTINATION ${PLUGIN_BASE_DIR}/lib
        )
    elseif(EXISTS "${OPENSSL_ROOT_DIR}/bin/libcrypto-3-x64.dll")
        install(FILES 
            "${OPENSSL_ROOT_DIR}/bin/libcrypto-3-x64.dll"
            "${OPENSSL_ROOT_DIR}/bin/libssl-3-x64.dll"
            DESTINATION ${PLUGIN_BASE_DIR}/lib
        )
    else()
        message(WARNING "[AYON] Could not find OpenSSL DLLs in ${OPENSSL_ROOT_DIR} to install. The plugin might fail to load.")
    endif()
endif()
