include_guard(GLOBAL)

message(STATUS "[AYON] Configuring resolver dependencies for Maya...")

set(MAYA_LIB_DIR "${MAYA_ROOT}/lib")
if(NOT EXISTS "${MAYA_LIB_DIR}")
    message(FATAL_ERROR "Maya lib directory not found: ${MAYA_LIB_DIR}")
endif()

find_library(MAYA_LIB_FOUNDATION Foundation HINTS "${MAYA_LIB_DIR}" REQUIRED)
find_library(MAYA_LIB_OPENMAYA OpenMaya HINTS "${MAYA_LIB_DIR}" REQUIRED)

if(NOT TARGET Maya::Core)
    add_library(Maya::Core UNKNOWN IMPORTED)
    set_target_properties(Maya::Core PROPERTIES
        IMPORTED_LOCATION "${MAYA_LIB_OPENMAYA}"
        INTERFACE_LINK_LIBRARIES "${MAYA_LIB_FOUNDATION}"
        INTERFACE_INCLUDE_DIRECTORIES "${MAYA_ROOT}/include"
        INTERFACE_COMPILE_DEFINITIONS "REQUIRE_IOSTREAM;_BOOL"
    )
endif()

if(NOT WIN32)
    set_property(TARGET Maya::Core APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS "LINUX")
endif()

if(NOT MAYA_USD_DEVKIT_PATH)
    message(FATAL_ERROR "MAYA_USD_DEVKIT_PATH must be set for Maya build.")
endif()

if(NOT TARGET Maya::USDDevkit)
    add_library(Maya::USDDevkit INTERFACE IMPORTED)
    set_target_properties(Maya::USDDevkit PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MAYA_USD_DEVKIT_PATH}/include"
    )
endif()

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
        set(Python_LIBRARIES "${MAYA_PY_LIB}")
        set(Python_VERSION "${_maya_pyver_dotted}")
        set(Python_FOUND TRUE)
        set(Python_Interpreter_FOUND TRUE)
        set(Python_Development_FOUND TRUE)
    else()
        set(MAYA_PY_INC "${MAYA_USD_DEVKIT_PATH}/include/Python${_maya_pyver}/Python")
        if(EXISTS "${MAYA_PY_INC}")
            set(Python_INCLUDE_DIRS "${MAYA_PY_INC}")
            set(Python_LIBRARIES "${MAYA_PY_LIB}")
            set(Python_FOUND TRUE)
        endif()
    endif()
else()
    if(DEFINED Python_INCLUDE_DIR AND Python_INCLUDE_DIR)
        set(Python_INCLUDE_DIRS "${Python_INCLUDE_DIR}")
    else()
        set(Python_INCLUDE_DIRS "${MAYA_ROOT}/include/Python${_maya_pyver}/Python")
    endif()

    if(NOT Python_LIBRARIES AND DEFINED Python_LIBRARY AND Python_LIBRARY)
        set(Python_LIBRARIES "${Python_LIBRARY}")
    endif()
    if(NOT Python_LIBRARIES)
        set(Python_LIBRARIES "${MAYA_ROOT}/lib/python${_maya_pyver}.lib")
    endif()

    set(Python_FOUND TRUE)
    set(Python_Interpreter_FOUND TRUE)
    set(Python_Development_FOUND TRUE)
    set(Python_Development_Module_FOUND TRUE)
    set(Python_Development_Embed_FOUND TRUE)

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

if(EXISTS "${USD_INCLUDE_DIR}/pxr/external/boost")
    find_package(Boost REQUIRED)
    set(AR_BOOST_INCLUDE_DIR "${Boost_INCLUDE_DIRS}")
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
    list(APPEND AYON_RESOLVER_DCC_COMPILE_DEFINITIONS AYON_USE_BOOST)
    message(STATUS "[AYON] Using plain boost for Maya: ${_maya_boost_inc}")
endif()

list(APPEND AYON_RESOLVER_DCC_INCLUDE_DIRS
    "${Python_INCLUDE_DIRS}"
    "${Boost_INCLUDE_DIRS}"
    "${AR_BOOST_INCLUDE_DIR}"
)
list(APPEND AYON_RESOLVER_DCC_LINK_LIBS Maya::Core Maya::USDDevkit)
if(USD_PYTHON_LIBRARIES)
    list(APPEND AYON_RESOLVER_DCC_LINK_LIBS "${USD_PYTHON_LIBRARIES}")
endif()
