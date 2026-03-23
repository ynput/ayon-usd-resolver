cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()

project(AyonUsdResolver)

# =========================================================
# GLOBAL COMPILER FLAGS (CRITICAL ABI FIXES)
# =========================================================
message(STATUS "[FORCE] Applying Houdini Namespace and Iterator fixes globally.")

add_compile_definitions(PXR_SET_INTERNAL_NAMESPACE)
add_compile_definitions(HBOOST_ALL_NO_LIB)

if(MSVC)
    add_compile_definitions("_ITERATOR_DEBUG_LEVEL=0")
    add_compile_definitions("TBB_USE_ASSERT=0")
    add_compile_definitions("TBB_USE_THREADING_TOOLS=0")
    add_compile_definitions("__TBB_NO_IMPLICIT_LINKAGE")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CORE_LIB_TARGET AyonUsdResolver)
set(PYTHON_MOD_TARGET AyonUsdResolver_python)

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
# Re-detect Houdini layout
# (_usd_is_houdini is local to FindUSD.cmake scope, re-detect here)
# =========================================================
if(USD_ROOT AND EXISTS "${USD_ROOT}/toolkit/include" AND (EXISTS "${USD_ROOT}/dsolib" OR EXISTS "${USD_ROOT}/custom/houdini/dsolib"))
    set(_usd_is_houdini TRUE)
else()
    set(_usd_is_houdini FALSE)
endif()
message(STATUS "[AYON] _usd_is_houdini = ${_usd_is_houdini}")

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
if(BUILD_TARGET STREQUAL "maya")
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
        set_property(TARGET Maya::Core APPEND PROPERTY
            INTERFACE_COMPILE_DEFINITIONS "LINUX")
    endif()

    if(NOT MAYA_USD_DEVKIT_PATH)
        message(FATAL_ERROR "MAYA_USD_DEVKIT_PATH must be set for Maya build.")
    endif()

    add_library(Maya::USDDevkit INTERFACE IMPORTED)
    set_target_properties(Maya::USDDevkit PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MAYA_USD_DEVKIT_PATH}/include"
    )

    if(Python_EXECUTABLE)
        set(_maya_pyexec "${Python_EXECUTABLE}")
    elseif(WIN32)
        set(_maya_pyexec "${MAYA_ROOT}/bin/mayapy.exe")
    else()
        set(_maya_pyexec "${MAYA_ROOT}/bin/mayapy")
    endif()
    execute_process(
        COMMAND "${_maya_pyexec}" -c
            "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')"
        OUTPUT_VARIABLE _maya_pyver
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _pyver_result
    )
    if(NOT _pyver_result EQUAL 0 OR NOT _maya_pyver)
        set(_maya_pyver "311")
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

        set(MAYA_USD_PYTHON_LIB "${USD_ROOT}/lib/usd_python.lib")
        if(EXISTS "${MAYA_USD_PYTHON_LIB}")
            set(USD_PYTHON_LIBRARIES "${MAYA_USD_PYTHON_LIB}")
            message(STATUS "[AYON] Found Maya USD python lib: ${MAYA_USD_PYTHON_LIB}")
        else()
            message(STATUS "[AYON] usd_python.lib not found, trying boost_python fallback...")
            message(STATUS "[AYON] Searching boost_python in: ${USD_LIB_DIR}")
            message(STATUS "[AYON] Searching boost_python in: ${MAYA_USD_DEVKIT_PATH}/lib")
            message(STATUS "[AYON] Searching boost_python in: ${MAYA_ROOT}/devkit/lib")

            find_library(_boost_python_lib
                NAMES
                    boost_python${_maya_pyver}-vc143-mt-x64-1_81
                    boost_python${_maya_pyver}-vc143-mt-x64
                    boost_python${_maya_pyver}-vc142-mt-x64-1_76
                    boost_python${_maya_pyver}-vc142-mt-x64
                    boost_python${_maya_pyver}
                PATHS
                    "${USD_LIB_DIR}"
                    "${MAYA_USD_DEVKIT_PATH}/lib"
                    "${MAYA_ROOT}/devkit/lib"
                NO_DEFAULT_PATH
            )

            if(_boost_python_lib)
                set(USD_PYTHON_LIBRARIES "${_boost_python_lib}")
                message(STATUS "[AYON] Using boost_python fallback: ${_boost_python_lib}")
            else()
                message(FATAL_ERROR
                    "[AYON] Could not find usd_python.lib or a compatible boost_python${_maya_pyver} library")
            endif()
        endif()
    endif()

    list(APPEND CMAKE_PREFIX_PATH "${MAYA_USD_DEVKIT_PATH}")
endif()

# =========================================================
# Houdini USD / Boost / Python Setup
# =========================================================
if(_usd_is_houdini)
    message(STATUS "[AYON] Configuring libraries for Houdini...")

    # ----------------------------------------------------------
    # Step 1: detect Houdini Python version + ABI flag
    # Houdini 19.5 = Python 3.7m, 20.0 = 3.9, 20.5 = 3.10, 21+ = 3.11
    # ----------------------------------------------------------
    if(NOT WIN32)
        # Extract the Houdini version from the dcc-root path (e.g., /opt/hfs19.5.716)
        string(REGEX MATCH "hfs([0-9]+\\.[0-9]+)" HOU_VER_MATCH "${USD_ROOT}")
        set(HOU_VER "${CMAKE_MATCH_1}")

        if(HOU_VER AND HOU_VER VERSION_LESS "20.0")
            message(STATUS "[AYON] Houdini ${HOU_VER} detected. Forcing old C++11 ABI (_GLIBCXX_USE_CXX11_ABI=0).")
            add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
        else()
            message(STATUS "[AYON] Houdini 20.0+ detected. Using default modern C++11 ABI.")
        endif()
    endif()

    if(Python_EXECUTABLE AND EXISTS "${Python_EXECUTABLE}")
        set(_hou_pyexec "${Python_EXECUTABLE}")
    elseif(WIN32)
        set(_hou_pyexec "${USD_ROOT}/python/python.exe")
    else()
        set(_hou_pyexec "${USD_ROOT}/python/bin/python")
    endif()

    execute_process(
        COMMAND "${_hou_pyexec}" -c
            "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')"
        OUTPUT_VARIABLE _hou_pyver_short
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND "${_hou_pyexec}" -c
            "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')"
        OUTPUT_VARIABLE _hou_pyver_dotted
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    # ABI flags: Python <=3.7 uses "m" suffix (libpython3.7m.so, dir python3.7m)
    execute_process(
        COMMAND "${_hou_pyexec}" -c
            "import sysconfig; print(sysconfig.get_config_var('ABIFLAGS') or '')"
        OUTPUT_VARIABLE _hou_pyabi
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT _hou_pyver_short)
        set(_hou_pyver_short "311")
    endif()
    if(NOT _hou_pyver_dotted)
        set(_hou_pyver_dotted "3.11")
    endif()
    if(NOT _hou_pyabi)
        set(_hou_pyabi "")
    endif()
    message(STATUS "[AYON] Houdini Python: ${_hou_pyver_dotted} (ABI flags: '${_hou_pyabi}')")

    # ----------------------------------------------------------
    # Step 2: Python include dir (handle python3.7m vs python3.11)
    # ----------------------------------------------------------
    if(NOT WIN32)
        # Prefer directory with ABI suffix (e.g. python3.7m), fall back without
        if(EXISTS "${USD_ROOT}/toolkit/include/python${_hou_pyver_dotted}${_hou_pyabi}")
            set(Python_INCLUDE_DIRS
                "${USD_ROOT}/toolkit/include/python${_hou_pyver_dotted}${_hou_pyabi}")
        else()
            set(Python_INCLUDE_DIRS
                "${USD_ROOT}/toolkit/include/python${_hou_pyver_dotted}")
        endif()

        # ----------------------------------------------------------
        # Step 3: Python library (handles 3.7m ABI suffix and .a static)
        # ----------------------------------------------------------
        find_library(HOUDINI_PYTHON_LIB
            NAMES
                python${_hou_pyver_dotted}${_hou_pyabi}
                python${_hou_pyver_dotted}
                python${_hou_pyver_short}${_hou_pyabi}
                python${_hou_pyver_short}
            PATHS
                "${USD_ROOT}/python/lib"
                "${USD_ROOT}/dsolib"
            NO_DEFAULT_PATH
        )
        set(Python_LIBRARIES "${HOUDINI_PYTHON_LIB}")
        set(Python_FOUND TRUE)
        set(Python_Interpreter_FOUND TRUE)
        set(Python_Development_FOUND TRUE)

        message(STATUS "[AYON] Bypassed FindPython for Houdini Linux.")
        message(STATUS "  -> Includes: ${Python_INCLUDE_DIRS}")
        message(STATUS "  -> Libs: ${Python_LIBRARIES}")
    endif()

    # ----------------------------------------------------------
    # Step 2b: Windows Houdini Python setup
    # ----------------------------------------------------------
    if(WIN32)
        # Set Windows Python paths based on detected version
        set(Python_INCLUDE_DIRS "${USD_ROOT}/python${_hou_pyver_short}/include")
        find_library(HOUDINI_PYTHON_LIB
            NAMES python${_hou_pyver_short}
            PATHS "${USD_ROOT}/python${_hou_pyver_short}/libs"
            NO_DEFAULT_PATH
        )
        set(Python_LIBRARIES "${HOUDINI_PYTHON_LIB}")
        set(Python_FOUND TRUE)
        set(Python_Interpreter_FOUND TRUE)
        set(Python_Development_FOUND TRUE)

        message(STATUS "[AYON] Bypassed FindPython for Houdini Windows.")
        message(STATUS "  -> Includes: ${Python_INCLUDE_DIRS}")
        message(STATUS "  -> Libs: ${Python_LIBRARIES}")
    endif()

    # ----------------------------------------------------------
    # Step 4: find hboost_python / pxr_python lib
    # ----------------------------------------------------------
    find_library(HOUDINI_PXR_PYTHON_LIB
        NAMES pxr_python libpxr_python
        PATHS "${USD_LIB_DIR}"
        NO_DEFAULT_PATH
    )
    if(HOUDINI_PXR_PYTHON_LIB)
        set(USD_PYTHON_LIBRARIES ${HOUDINI_PXR_PYTHON_LIB})
        message(STATUS "[AYON] Found pxr_python: ${HOUDINI_PXR_PYTHON_LIB}")
    else()
        find_library(HOUDINI_HBOOST_PYTHON_LIB
            NAMES
                # Houdini 20.5 / 21.0 — no ABI flag
                hboost_python${_hou_pyver_short}-mt-x64
                hboost_python${_hou_pyver_short}
                # Houdini 19.5 — with ABI flag (e.g. hboost_python37m-mt-x64)
                hboost_python${_hou_pyver_short}${_hou_pyabi}-mt-x64
                hboost_python${_hou_pyver_short}${_hou_pyabi}
                # dotted variants
                hboost_python${_hou_pyver_dotted}-mt-x64
                hboost_python${_hou_pyver_dotted}
            PATHS "${USD_LIB_DIR}"
            NO_DEFAULT_PATH
        )
        if(HOUDINI_HBOOST_PYTHON_LIB)
            set(USD_PYTHON_LIBRARIES ${HOUDINI_HBOOST_PYTHON_LIB})
            message(STATUS "[AYON] Found hboost_python: ${HOUDINI_HBOOST_PYTHON_LIB}")
        else()
            message(WARNING
                "[AYON] Could not find pxr_python or hboost_python in ${USD_LIB_DIR} — "
                "Python module linking may fail.")
            set(USD_PYTHON_LIBRARIES "")
        endif()
    endif()

    set(HOUDINI_SPDLOG_INCLUDE "${USD_ROOT}/toolkit/include")

    # ----------------------------------------------------------
    # Step 5: boost/hboost include layout detection
    # Probe for python.hpp (more reliable than directory existence)
    # ----------------------------------------------------------
    if(EXISTS "${USD_INCLUDE_DIR}/pxr/external/boost/python.hpp")
        # Houdini 21.0+ (pxr_boost)
        set(AR_BOOST_NAMESPACE pxr_boost)
        set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}/pxr/external")
        message(STATUS "[AYON] hboost layout: pxr_boost in pxr/external")
    elseif(EXISTS "${USD_INCLUDE_DIR}/pxr/external/hboost/python.hpp")
        # Houdini 20.0 / 20.5 (hboost inside pxr/external)
        set(AR_BOOST_NAMESPACE hboost)
        set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}/pxr/external")
        add_compile_definitions(AYON_USE_HBOOST)
        message(STATUS "[AYON] hboost layout: hboost in pxr/external")
    elseif(EXISTS "${USD_INCLUDE_DIR}/hboost/python.hpp")
        # Houdini 19.5 and older (hboost directly under toolkit/include)
        set(AR_BOOST_NAMESPACE hboost)
        set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}")
        add_compile_definitions(AYON_USE_HBOOST)
        message(STATUS "[AYON] hboost layout: hboost directly in toolkit/include")
    else()
        # Fallback: hboost dir exists but Python sub-headers missing
        # (Houdini 20.5 ships hboost without Python binding headers)
        # The stub at src/AyonUsdResolver/hboost/python/detail/wrap_python.hpp
        # covers the missing header via CMAKE_CURRENT_SOURCE_DIR being first in includes.
        set(AR_BOOST_NAMESPACE hboost)
        set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}")
        add_compile_definitions(AYON_USE_HBOOST)
        message(STATUS "[AYON] hboost layout: fallback — stub covers wrap_python.hpp")
    endif()

    set(Boost_FOUND TRUE)
    set(Boost_INCLUDE_DIRS "${AR_BOOST_INCLUDE_DIR}")
    message(STATUS "[AYON] Boost include dir: ${AR_BOOST_INCLUDE_DIR}")

elseif(BUILD_TARGET STREQUAL "maya")
    if(EXISTS "${USD_INCLUDE_DIR}/pxr/external/boost")
        find_package(Boost REQUIRED)
    else()
        set(_maya_boost_candidates
            "${MAYA_USD_DEVKIT_PATH}/include/boost-1_85"
            "${MAYA_USD_DEVKIT_PATH}/include/boost-1_82"
            "${MAYA_USD_DEVKIT_PATH}/include/boost-1_81"
            "${MAYA_USD_DEVKIT_PATH}/include/boost-1_76"
            "${MAYA_USD_DEVKIT_PATH}/include"
            "${USD_ROOT}/include/boost-1_85"
            "${USD_ROOT}/include/boost-1_82"
            "${USD_ROOT}/include/boost-1_81"
            "${USD_ROOT}/include/boost-1_76"
            "${USD_ROOT}/include"
        )

        unset(_maya_boost_inc)
        foreach(_boost_inc IN LISTS _maya_boost_candidates)
            if(EXISTS "${_boost_inc}/boost/python/detail/wrap_python.hpp")
                set(_maya_boost_inc "${_boost_inc}")
                break()
            endif()
        endforeach()

        if(NOT _maya_boost_inc)
            message(FATAL_ERROR
                "[AYON] Could not find Maya Boost headers. "
                "Expected boost/python/detail/wrap_python.hpp under Maya devkit or USD root."
            )
        endif()

        set(Boost_FOUND TRUE)
        set(Boost_INCLUDE_DIRS "${_maya_boost_inc}")
        set(AR_BOOST_INCLUDE_DIR "${_maya_boost_inc}")
        add_compile_definitions(AYON_USE_BOOST)
        message(STATUS "[AYON] Using plain boost for Maya: ${_maya_boost_inc}")
    endif()
else()
    find_package(Boost REQUIRED)
endif()

# =========================================================
# PYTHON (non-Houdini paths)
# =========================================================
if(WIN32 AND BUILD_TARGET STREQUAL "maya")

    message(STATUS "[AYON] Bypassing FindPython for Maya on Windows.")

    if(NOT Python_EXECUTABLE)
        if(DEFINED PYTHON_EXECUTABLE)
            set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
        else()
            set(Python_EXECUTABLE "${MAYA_ROOT}/bin/mayapy.exe")
        endif()
    endif()

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

    set(Python_FOUND TRUE)
    set(Python_Interpreter_FOUND TRUE)
    set(Python_Development_FOUND TRUE)
    set(Python_Development_Module_FOUND TRUE)
    set(Python_Development_Embed_FOUND TRUE)

    if(NOT TARGET Python::Python)
        add_library(Python::Python UNKNOWN IMPORTED)
        set_target_properties(Python::Python PROPERTIES
            IMPORTED_LOCATION "${Python_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${Python_INCLUDE_DIRS}"
        )
    endif()

elseif(NOT Python_FOUND)
    # Standard FindPython for standalone USD builds
    if(DEFINED Python_EXECUTABLE)
        set(_active_py_exec ${Python_EXECUTABLE})
    elseif(DEFINED PYTHON_EXECUTABLE)
        set(_active_py_exec ${PYTHON_EXECUTABLE})
        set(Python_EXECUTABLE ${_active_py_exec})
    endif()

    if(DEFINED _active_py_exec AND NOT BUILD_TARGET STREQUAL "maya")
        get_filename_component(_python_root "${_active_py_exec}" DIRECTORY)
        get_filename_component(_python_root "${_python_root}" DIRECTORY)
        set(Python_ROOT_DIR "${_python_root}")
    endif()

    find_package(Python COMPONENTS Interpreter Development REQUIRED)
endif()

# =========================================================
# INCLUDE DIRECTORIES
# =========================================================
message(STATUS "[AYON] Final AR_BOOST_INCLUDE_DIR: ${AR_BOOST_INCLUDE_DIR}")
message(STATUS "[AYON] Final Python_INCLUDE_DIRS:  ${Python_INCLUDE_DIRS}")

set(RESOLVER_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${USD_INCLUDE_DIR}
    ${AR_BOOST_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${Python_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src/lib
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include
    ${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/spdlog/include
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
    Wrappers_Tokens/resolverTokens.cpp
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

if(MSVC)
    target_compile_options(${CORE_LIB_TARGET} PRIVATE /Zc:inline-)
    target_link_options(${CORE_LIB_TARGET} PRIVATE /OPT:NOREF)
endif()

# =========================================================
# Target: Python Module
# =========================================================
add_library(${PYTHON_MOD_TARGET} MODULE
    Wrappers_Tokens/wrapResolverContext.cpp
    Wrappers_Tokens/wrapResolver.cpp
    Wrappers_Tokens/wrapResolverTokens.cpp
    Wrappers_Tokens/resolverTokens.cpp
    pluginData/module.cpp
)

target_include_directories(${PYTHON_MOD_TARGET} PRIVATE ${RESOLVER_INCLUDE_DIRS})
target_compile_definitions(${PYTHON_MOD_TARGET} PRIVATE AR_AYONUSDRESOLVER_EXPORTS)

if(MSVC)
    target_compile_options(${PYTHON_MOD_TARGET} PRIVATE /Zc:inline-)
    target_link_options(${PYTHON_MOD_TARGET} PRIVATE /OPT:NOREF)
endif()

# =========================================================
# Locate additional core USD libraries (sdf, vt, pcp)
# =========================================================
set(USD_EXTRA_LIBS "")
if(_usd_is_houdini OR BUILD_TARGET STREQUAL "maya")
    set(_usd_lib_names sdf vt pcp)
    foreach(_libname IN LISTS _usd_lib_names)
        find_library(_usd_lib_found
            NAMES ${_usd_prefix}_${_libname} lib${_usd_prefix}_${_libname}
            PATHS "${USD_LIB_DIR}"
            NO_DEFAULT_PATH
        )
        if(_usd_lib_found)
            list(APPEND USD_EXTRA_LIBS ${_usd_lib_found})
            message(STATUS "[AYON] Found USD library: ${_libname} = ${_usd_lib_found}")
            unset(_usd_lib_found CACHE)
        endif()
    endforeach()
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
        ${USD_EXTRA_LIBS}
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
        ${USD_EXTRA_LIBS}
        ${USD_PYTHON_LIBRARIES}
)

if(BUILD_TARGET STREQUAL "maya")
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
    set_target_properties(${PYTHON_MOD_TARGET} PROPERTIES
        PREFIX "" SUFFIX ".pyd" OUTPUT_NAME "_ayonUsdResolver")
else()
    set(AR_ARCH_LIB_SUFFIX "so")
    set_target_properties(${PYTHON_MOD_TARGET} PROPERTIES
        PREFIX "" SUFFIX ".so" OUTPUT_NAME "_ayonUsdResolver")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json.in
    ${CMAKE_BINARY_DIR}/plugInfo.json
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/plugInfo.json
    DESTINATION ${PLUGIN_BASE_DIR}/resources)
install(TARGETS ${CORE_LIB_TARGET}
    DESTINATION ${PLUGIN_BASE_DIR}/lib)
install(TARGETS ${PYTHON_MOD_TARGET}
    DESTINATION ${PLUGIN_BASE_DIR}/lib/python/usdAssetResolver/AyonUsdResolver)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
        DESTINATION ${PLUGIN_BASE_DIR}/lib/python/usdAssetResolver/AyonUsdResolver)
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
        message(WARNING
            "[AYON] Could not find OpenSSL DLLs in ${OPENSSL_ROOT_DIR} to install. "
            "The plugin might fail to load.")
    endif()
endif()
