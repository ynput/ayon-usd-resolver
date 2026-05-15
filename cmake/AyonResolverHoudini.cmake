include_guard(GLOBAL)

message(STATUS "[AYON] Configuring resolver dependencies for Houdini...")

if(NOT WIN32)
    string(REGEX MATCH "hfs([0-9]+\\.[0-9]+)" HOU_VER_MATCH "${USD_ROOT}")
    set(HOU_VER "${CMAKE_MATCH_1}")

    if(HOU_VER AND HOU_VER VERSION_LESS "20.0")
        message(STATUS "[AYON] Houdini ${HOU_VER} detected. Forcing old C++11 ABI (_GLIBCXX_USE_CXX11_ABI=0).")
        list(APPEND AYON_RESOLVER_DCC_COMPILE_DEFINITIONS _GLIBCXX_USE_CXX11_ABI=0)
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

if(NOT WIN32)
    if(EXISTS "${USD_ROOT}/toolkit/include/python${_hou_pyver_dotted}${_hou_pyabi}")
        set(Python_INCLUDE_DIRS
            "${USD_ROOT}/toolkit/include/python${_hou_pyver_dotted}${_hou_pyabi}")
    else()
        set(Python_INCLUDE_DIRS
            "${USD_ROOT}/toolkit/include/python${_hou_pyver_dotted}")
    endif()

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
else()
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

find_library(HOUDINI_PXR_PYTHON_LIB
    NAMES pxr_python libpxr_python
    PATHS "${USD_LIB_DIR}"
    NO_DEFAULT_PATH
)
if(HOUDINI_PXR_PYTHON_LIB)
    set(USD_PYTHON_LIBRARIES "${HOUDINI_PXR_PYTHON_LIB}")
    message(STATUS "[AYON] Found pxr_python: ${HOUDINI_PXR_PYTHON_LIB}")
else()
    find_library(HOUDINI_HBOOST_PYTHON_LIB
        NAMES
            hboost_python${_hou_pyver_short}-mt-x64
            hboost_python${_hou_pyver_short}
            hboost_python${_hou_pyver_short}${_hou_pyabi}-mt-x64
            hboost_python${_hou_pyver_short}${_hou_pyabi}
            hboost_python${_hou_pyver_dotted}-mt-x64
            hboost_python${_hou_pyver_dotted}
        PATHS "${USD_LIB_DIR}"
        NO_DEFAULT_PATH
    )
    if(HOUDINI_HBOOST_PYTHON_LIB)
        set(USD_PYTHON_LIBRARIES "${HOUDINI_HBOOST_PYTHON_LIB}")
        message(STATUS "[AYON] Found hboost_python: ${HOUDINI_HBOOST_PYTHON_LIB}")
    else()
        message(WARNING
            "[AYON] Could not find pxr_python or hboost_python in ${USD_LIB_DIR} - "
            "Python module linking may fail.")
        set(USD_PYTHON_LIBRARIES "")
    endif()
endif()

set(HOUDINI_SPDLOG_INCLUDE "${USD_ROOT}/toolkit/include")

if(EXISTS "${USD_INCLUDE_DIR}/pxr/external/boost/python.hpp")
    set(AR_BOOST_NAMESPACE pxr_boost)
    set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}/pxr/external")
    message(STATUS "[AYON] hboost layout: pxr_boost in pxr/external")
elseif(EXISTS "${USD_INCLUDE_DIR}/pxr/external/hboost/python.hpp")
    set(AR_BOOST_NAMESPACE hboost)
    set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}/pxr/external")
    list(APPEND AYON_RESOLVER_DCC_COMPILE_DEFINITIONS AYON_USE_HBOOST)
    message(STATUS "[AYON] hboost layout: hboost in pxr/external")
elseif(EXISTS "${USD_INCLUDE_DIR}/hboost/python.hpp")
    set(AR_BOOST_NAMESPACE hboost)
    set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}")
    list(APPEND AYON_RESOLVER_DCC_COMPILE_DEFINITIONS AYON_USE_HBOOST)
    message(STATUS "[AYON] hboost layout: hboost directly in toolkit/include")
else()
    set(AR_BOOST_NAMESPACE hboost)
    set(AR_BOOST_INCLUDE_DIR "${USD_INCLUDE_DIR}")
    list(APPEND AYON_RESOLVER_DCC_COMPILE_DEFINITIONS AYON_USE_HBOOST)
    message(STATUS "[AYON] hboost layout: fallback - stub covers wrap_python.hpp")
endif()

list(APPEND AYON_RESOLVER_DCC_INCLUDE_DIRS
    "${AR_BOOST_INCLUDE_DIR}"
    "${Python_INCLUDE_DIRS}"
    "${HOUDINI_SPDLOG_INCLUDE}"
)

if(USD_PYTHON_LIBRARIES)
    list(APPEND AYON_RESOLVER_DCC_LINK_LIBS "${USD_PYTHON_LIBRARIES}")
endif()
