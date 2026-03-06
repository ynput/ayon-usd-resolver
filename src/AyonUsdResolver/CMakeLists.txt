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

    # =========================================================
    # Auto-detect Maya Python version from the executable
    # =========================================================
    if(Python_EXECUTABLE)
        set(_maya_pyexec "${Python_EXECUTABLE}")
    elseif(WIN32)
        set(_maya_pyexec "${MAYA_ROOT}/bin/mayapy.exe")
    else()
        set(_maya_pyexec "${MAYA_ROOT}/bin/mayapy")
    endif()
    execute_process(
        COMMAND "${_maya_pyexec}" -c "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')"
        OUTPUT_VARIABLE _maya_pyver OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET RESULT_VARIABLE _pyver_result
    )
    if(NOT _pyver_result EQUAL 0 OR NOT _maya_pyver)
        set(_maya_pyver "311")  # fallback
    endif()
    string(SUBSTRING "${_maya_pyver}" 0 1 _pyver_major)
    string(SUBSTRING "${_maya_pyver}" 1 -1 _pyver_minor)
    set(_maya_pyver_dotted "${_pyver_major}.${_pyver_minor}")
    message(STATUS "[AYON] Detected Maya Python version: ${_maya_pyver} (${_maya_pyver_dotted})")

    if(NOT WIN32)
        set(MAYA_PY_INC "${MAYA_ROOT}/include/Python${_maya_pyver}/Python")
        set(MAYA_PY_LIB "${MAYA_LIB_DIR}/libpython${_maya_pyver_dotted}.so")
        if(EXISTS "${MAYA_PY_INC}" AND EXISTS "${MAYA_PY_LIB}")
            set(Python_INCLUDE_DIRS "${MAYA_PY_INC}")
            set(Python_LIBRARIES    "${MAYA_PY_LIB}")
            set(Python_VERSION      "${_maya_pyver_dotted}")
            set(Python_FOUND TRUE)
            set(Python_Interpreter_FOUND TRUE)
            set(Python_Development_FOUND TRUE)
        else()
            set(MAYA_PY_INC "${MAYA_USD_DEVKIT_PATH}/include/Python${_maya_pyver}/Python")
            if(EXISTS "${MAYA_PY_INC}")
                set(Python_INCLUDE_DIRS "${MAYA_PY_INC}")
                set(Python_LIBRARIES    "${MAYA_PY_LIB}")
                set(Python_FOUND TRUE)
            endif()
        endif()
    endif()

    if(WIN32)
        set(MAYA_PYTHON_INCLUDE_HINT "${MAYA_ROOT}/include/Python${_maya_pyver}/Python")
        list(APPEND CMAKE_INCLUDE_PATH "${MAYA_PYTHON_INCLUDE_HINT}")
        set(Python_INCLUDE_DIR "${MAYA_PYTHON_INCLUDE_HINT}")

        # =========================================================
        # [FIX] Maya (Windows): link USD's python support lib
        # Try usd_python.lib first (Maya 2026+), fall back to boost_python (Maya 2024)
        # =========================================================
        set(MAYA_USD_PYTHON_LIB "${USD_ROOT}/lib/usd_python.lib")
        if(EXISTS "${MAYA_USD_PYTHON_LIB}")
            set(USD_PYTHON_LIBRARIES "${MAYA_USD_PYTHON_LIB}")
            message(STATUS "[AYON] Found Maya USD python lib: ${MAYA_USD_PYTHON_LIB}")
        else()
            # Fall back to boost_python from the devkit
            find_library(_boost_python_lib
                NAMES boost_python${_maya_pyver}-vc142-mt-x64-1_76
                      boost_python${_maya_pyver}
                PATHS "${USD_LIB_DIR}" "${MAYA_USD_DEVKIT_PATH}/lib"
                NO_DEFAULT_PATH
            )
            if(_boost_python_lib)
                set(USD_PYTHON_LIBRARIES "${_boost_python_lib}")
                message(STATUS "[AYON] Using boost_python fallback: ${_boost_python_lib}")
            else()
                message(FATAL_ERROR "[AYON] Could not find usd_python.lib or boost_python${_maya_pyver} lib")
            endif()
        endif()
    endif()

    list(APPEND CMAKE_PREFIX_PATH "${MAYA_USD_DEVKIT_PATH}")
endif()

# =========================================================
# Houdini USD / Boost Setup
# =========================================================
if(_usd_is_houdini)
    message(STATUS "[AYON] Configuring libraries for Houdini...")

    # Note: Global flags (PXR_SET_INTERNAL_NAMESPACE, etc.) are already set at the top.

    # [FIX] Find python boost/pxr lib to prevent unresolved symbol errors.
    # Uses USD_LIB_DIR (resolved per-platform by FindUSD.cmake) for portability:
    #   Windows: custom/houdini/dsolib/   Linux/Mac: dsolib/
    # Strategy: try pxr_python first (Houdini 21+), fall back to hboost_python (Houdini 20.5)
    find_library(HOUDINI_PXR_PYTHON_LIB
        NAMES pxr_python libpxr_python
        PATHS "${USD_LIB_DIR}"
        NO_DEFAULT_PATH
    )
    if(HOUDINI_PXR_PYTHON_LIB)
        set(USD_PYTHON_LIBRARIES ${HOUDINI_PXR_PYTHON_LIB})
        message(STATUS "[AYON] Found pxr_python: ${HOUDINI_PXR_PYTHON_LIB}")
    else()
        # Houdini 20.5 and earlier bundle hboost_python instead
        find_library(HOUDINI_HBOOST_PYTHON_LIB
            NAMES hboost_python311-mt-x64 hboost_python310-mt-x64 hboost_python39-mt-x64
                  hboost_python311 hboost_python310 hboost_python39
                  hboost_python3.11 hboost_python3.10 hboost_python3.9
            PATHS "${USD_LIB_DIR}"
            NO_DEFAULT_PATH
        )
        if(HOUDINI_HBOOST_PYTHON_LIB)
            set(USD_PYTHON_LIBRARIES ${HOUDINI_HBOOST_PYTHON_LIB})
            message(STATUS "[AYON] Found hboost_python: ${HOUDINI_HBOOST_PYTHON_LIB}")
        else()
            message(WARNING "[AYON] Could not find pxr_python or hboost_python in ${USD_LIB_DIR}! Linking may fail.")
        endif()
    endif()
    set(HOUDINI_SPDLOG_INCLUDE "${USD_ROOT}/toolkit/include")

    if(EXISTS "${AR_PXR_INCLUDE_DIR}/pxr/external/boost")
        set(AR_BOOST_NAMESPACE pxr_boost)
        set(AR_BOOST_INCLUDE_DIR ${AR_PXR_INCLUDE_DIR}/pxr/external)
    else()
        set(AR_BOOST_NAMESPACE hboost)
        set(AR_BOOST_INCLUDE_DIR ${USD_INCLUDE_DIR})
        # Signal to source files that we use hboost instead of pxr_boost
        add_compile_definitions(AYON_USE_HBOOST)
    endif()

    set(Boost_FOUND TRUE)
    set(Boost_INCLUDE_DIRS ${AR_BOOST_INCLUDE_DIR})
elseif(BUILD_TARGET STREQUAL "maya")
    # Maya USD: detect boost layout
    if(EXISTS "${USD_INCLUDE_DIR}/pxr/external/boost")
        # USD 24+ style (Maya 2026+)
        find_package(Boost REQUIRED)
    else()
        # USD 22.x style (Maya 2024) — plain boost from devkit
        set(_maya_boost_inc "${MAYA_USD_DEVKIT_PATH}/include/boost-1_76")
        if(NOT EXISTS "${_maya_boost_inc}")
            set(_maya_boost_inc "${MAYA_USD_DEVKIT_PATH}/include")
        endif()
        set(Boost_FOUND TRUE)
        set(Boost_INCLUDE_DIRS "${_maya_boost_inc}")
        add_compile_definitions(AYON_USE_BOOST)
        message(STATUS "[AYON] Using plain boost for Maya (USD 22.x): ${_maya_boost_inc}")
    endif()
else()
    find_package(Boost REQUIRED)
endif()

# =========================================================
# PYTHON
# =========================================================
if(WIN32 AND BUILD_TARGET STREQUAL "maya")

    message(STATUS "[AYON] Bypassing FindPython for Maya on Windows (using provided cache vars).")

    if(NOT Python_EXECUTABLE)
        # If caller passed PYTHON_EXECUTABLE instead, accept it.
        if(DEFINED PYTHON_EXECUTABLE)
            set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
        else()
            set(Python_EXECUTABLE "${MAYA_ROOT}/bin/mayapy.exe")
        endif()
    endif()

    # Prefer Python_INCLUDE_DIRS if already set (we set it above to MAYA_ROOT)
    if(NOT Python_INCLUDE_DIRS)
        if(DEFINED Python_INCLUDE_DIR)
            set(Python_INCLUDE_DIRS "${Python_INCLUDE_DIR}")
        else()
            set(Python_INCLUDE_DIRS "${MAYA_ROOT}/include/Python${_maya_pyver}/Python")
        endif()
    endif()

    if(NOT Python_LIBRARIES)
        if(DEFINED Python_LIBRARY)
            set(Python_LIBRARIES "${Python_LIBRARY}")
        else()
            set(Python_LIBRARIES "${MAYA_ROOT}/lib/python${_maya_pyver}.lib")
        endif()
    endif()

    # Mark as found so downstream checks pass
    set(Python_FOUND TRUE)
    set(Python_Interpreter_FOUND TRUE)
    set(Python_Development_FOUND TRUE)
    set(Python_Development_Module_FOUND TRUE)
    set(Python_Development_Embed_FOUND TRUE)

    # Provide imported target if anything expects it
    if(NOT TARGET Python::Python)
        add_library(Python::Python UNKNOWN IMPORTED)
        set_target_properties(Python::Python PROPERTIES
            IMPORTED_LOCATION "${Python_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${Python_INCLUDE_DIRS}"
        )
    endif()

else()

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

if (MSVC)
    target_compile_options(${CORE_LIB_TARGET} PRIVATE /Zc:inline-)
    target_link_options(${CORE_LIB_TARGET} PRIVATE /OPT:NOREF)
    # Optional extra safety:
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
