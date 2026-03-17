message(STATUS "[AYON] Using custom FindUSD.cmake from ${CMAKE_CURRENT_LIST_DIR}")

if (TARGET USD::tf AND TARGET USD::ar)
    set(USD_FOUND TRUE)
    message(STATUS "USD targets already defined, skipping redefinition.")
    return()
endif()

# -----------------------------------------------------------------------------
# USD root detection
# -----------------------------------------------------------------------------
if (DEFINED USD_ROOT)
    message(STATUS "Using USD root from CMake variable: ${USD_ROOT}")
elseif (DEFINED ENV{AyonUsdRoot})
    set(USD_ROOT $ENV{AyonUsdRoot})
    message(STATUS "Using USD root from environment: ${USD_ROOT}")
else()
    message(FATAL_ERROR
        "USD root not specified.\n"
        "Please provide -DUSD_ROOT=/path/to/USD or set AyonUsdRoot.")
endif()

# -----------------------------------------------------------------------------
# Detect Houdini layout (19–21)
# -----------------------------------------------------------------------------
set(_usd_is_houdini FALSE)
# FIX: Removed the strict check for 'dsolib' at root, which might fail on Windows.
# Instead, we rely on the existence of the toolkit headers, which is consistent.
if (EXISTS "${USD_ROOT}/toolkit/include/pxr/pxr.h")
    set(_usd_is_houdini TRUE)
    message(STATUS "[AYON] Detected Houdini USD layout at ${USD_ROOT}")
endif()

# -----------------------------------------------------------------------------
# Locate include directory
# -----------------------------------------------------------------------------

# 1. Manual Override (Important: Allows Python script to pass the path if needed)
if (DEFINED USD_INCLUDE_DIR AND EXISTS "${USD_INCLUDE_DIR}/pxr/pxr.h")
    message(STATUS "[AYON] Using manually provided USD_INCLUDE_DIR: ${USD_INCLUDE_DIR}")

# 2. Check Houdini
elseif (_usd_is_houdini)
    set(USD_INCLUDE_DIR "${USD_ROOT}/toolkit/include")
elseif (EXISTS "${USD_ROOT}/../../../devkit/include/pxr/pxr.h")
    get_filename_component(_usd_common_ancestor "${USD_ROOT}/../../.." ABSOLUTE)
    set(USD_INCLUDE_DIR "${_usd_common_ancestor}/devkit/include")
    message(STATUS "[AYON] Detected MayaUSD Split Layout (Linux).")
    message(STATUS "       - Found headers at: ${USD_INCLUDE_DIR}")
elseif (DEFINED MAYA_USD_DEVKIT_PATH AND EXISTS "${MAYA_USD_DEVKIT_PATH}/include/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${MAYA_USD_DEVKIT_PATH}/include")
    message(STATUS "[AYON] Detected MayaUSD 0.34.0+ layout — headers in Maya devkit.")
    message(STATUS "       - Found headers at: ${USD_INCLUDE_DIR}")

# 4. Standard Layouts
elseif (EXISTS "${USD_ROOT}/include/maya-usd/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${USD_ROOT}/include/maya-usd")
    message(STATUS "[AYON] Found Include Dir (Maya USD): ${USD_INCLUDE_DIR}")
elseif (EXISTS "${USD_ROOT}/include/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${USD_ROOT}/include/maya-usd")
    message(STATUS "[AYON] Found Include Dir (Maya USD): ${USD_INCLUDE_DIR}")
elseif (EXISTS "${USD_ROOT}/include/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${USD_ROOT}/include")
elseif (EXISTS "${USD_ROOT}/../include/pxr/pxr.h")
    get_filename_component(_usd_parent "${USD_ROOT}" DIRECTORY)
    set(USD_INCLUDE_DIR "${_usd_parent}/include")
elseif (EXISTS "${USD_ROOT}/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${USD_ROOT}")
else()
    message(FATAL_ERROR
        "Could not find USD include directory under ${USD_ROOT}. "
        "Checked toolkit/include, ../../../devkit/include, include/maya-usd, include/, ../include/, and .")
endif()

# -----------------------------------------------------------------------------
# Locate library directory
# -----------------------------------------------------------------------------
if (EXISTS "${USD_ROOT}/dsolib")
    set(USD_LIB_DIR "${USD_ROOT}/dsolib")
elseif (EXISTS "${USD_ROOT}/custom/houdini/dsolib")
    set(USD_LIB_DIR "${USD_ROOT}/custom/houdini/dsolib")
    message(STATUS "[AYON] Found Windows Houdini Libs: ${USD_LIB_DIR}")
elseif (EXISTS "${USD_ROOT}/lib/usd")
    set(USD_LIB_DIR "${USD_ROOT}/lib/usd")
    message(STATUS "[AYON] Found MayaUSD lib/usd layout: ${USD_LIB_DIR}")
elseif (EXISTS "${USD_ROOT}/lib/maya-usd")
    set(USD_LIB_DIR "${USD_ROOT}/lib/maya-usd")
elseif (EXISTS "${USD_ROOT}/lib")
    set(USD_LIB_DIR "${USD_ROOT}/lib")
elseif (EXISTS "${USD_ROOT}/../dsolib")
    get_filename_component(_usd_parent "${USD_ROOT}" DIRECTORY)
    set(USD_LIB_DIR "${_usd_parent}/dsolib")
else()
    message(FATAL_ERROR
        "Could not find USD library directory under ${USD_ROOT}. "
        "Checked dsolib, custom/houdini/dsolib, lib/usd, lib/maya-usd and lib.")
endif()

# -----------------------------------------------------------------------------
# Detect library prefix (libusd_* vs libpxr_*)
# -----------------------------------------------------------------------------
set(_usd_prefix "usd")
# Added check for .lib (Windows) and .so/.a
if (EXISTS "${USD_LIB_DIR}/libpxr_tf.so" OR EXISTS "${USD_LIB_DIR}/libpxr_tf.a" OR EXISTS "${USD_LIB_DIR}/pxr_tf.lib" OR EXISTS "${USD_LIB_DIR}/libpxr_tf.lib")
    set(_usd_prefix "pxr")
endif()
message(STATUS "[AYON] Detected USD library prefix: ${_usd_prefix}_")

# -----------------------------------------------------------------------------
# Locate core libraries
# -----------------------------------------------------------------------------
find_library(USD_LIB_TF NAMES ${_usd_prefix}_tf lib${_usd_prefix}_tf PATHS "${USD_LIB_DIR}" NO_DEFAULT_PATH)
find_library(USD_LIB_AR NAMES ${_usd_prefix}_ar lib${_usd_prefix}_ar PATHS "${USD_LIB_DIR}" NO_DEFAULT_PATH)
find_library(USD_LIB_PLUG NAMES ${_usd_prefix}_plug lib${_usd_prefix}_plug PATHS "${USD_LIB_DIR}" NO_DEFAULT_PATH)
find_library(USD_LIB_ARCH NAMES ${_usd_prefix}_arch lib${_usd_prefix}_arch PATHS "${USD_LIB_DIR}" NO_DEFAULT_PATH)

if (NOT USD_LIB_TF OR NOT USD_LIB_AR OR NOT USD_LIB_PLUG OR NOT USD_LIB_ARCH)
    message(FATAL_ERROR
        "Could not find USD core libraries in ${USD_LIB_DIR}. "
        "Checked for ${_usd_prefix}_tf, ${_usd_prefix}_ar, ${_usd_prefix}_plug, ${_usd_prefix}_arch.")
endif()

# -----------------------------------------------------------------------------
# Create imported targets
# -----------------------------------------------------------------------------
add_library(USD::tf UNKNOWN IMPORTED)
set_target_properties(USD::tf PROPERTIES
    IMPORTED_LOCATION ${USD_LIB_TF}
    INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
)

add_library(USD::ar UNKNOWN IMPORTED)
set_target_properties(USD::ar PROPERTIES
    IMPORTED_LOCATION ${USD_LIB_AR}
    INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
)

add_library(USD::plug UNKNOWN IMPORTED)
set_target_properties(USD::plug PROPERTIES
    IMPORTED_LOCATION ${USD_LIB_PLUG}
    INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
)

add_library(USD::arch UNKNOWN IMPORTED)
set_target_properties(USD::arch PROPERTIES
    IMPORTED_LOCATION ${USD_LIB_ARCH}
    INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
)

# -----------------------------------------------------------------------------
# Export convenience variables
# -----------------------------------------------------------------------------
set(USD_FOUND TRUE)
set(USD_INCLUDE_DIR ${USD_INCLUDE_DIR} CACHE PATH "USD include directory")
set(USD_LIB_DIR ${USD_LIB_DIR} CACHE PATH "USD library directory")

# -----------------------------------------------------------------------------
# Diagnostics
# -----------------------------------------------------------------------------
message(STATUS "USD found: ${USD_INCLUDE_DIR}")
message(STATUS "USD libs: ${USD_LIB_DIR}")
message(STATUS "USD_LIB_TF: ${USD_LIB_TF}")
message(STATUS "USD_LIB_AR: ${USD_LIB_AR}")
message(STATUS "USD_LIB_PLUG: ${USD_LIB_PLUG}")
message(STATUS "USD_LIB_ARCH: ${USD_LIB_ARCH}")
