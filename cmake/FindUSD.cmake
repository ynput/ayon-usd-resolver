message(STATUS "[AYON] Using custom FindUSD.cmake from ${CMAKE_CURRENT_LIST_DIR}")

if (TARGET USD::tf AND TARGET USD::ar)
    set(USD_FOUND TRUE)
    message(STATUS "USD targets already defined, skipping redefinition.")
    return()
endif()

# =========================================================
# USD root detection
# =========================================================
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

# =========================================================
# Detect Houdini layout (19–21)
# =========================================================
set(_usd_is_houdini FALSE)
if (EXISTS "${USD_ROOT}/toolkit/include/pxr/pxr.h")
    set(_usd_is_houdini TRUE)
    message(STATUS "[AYON] Detected Houdini USD layout at ${USD_ROOT}")
endif()

# =========================================================
# Locate include directory
# =========================================================

# 1. Manual Override
if (DEFINED USD_INCLUDE_DIR AND EXISTS "${USD_INCLUDE_DIR}/pxr/pxr.h")
    message(STATUS "[AYON] Using manually provided USD_INCLUDE_DIR: ${USD_INCLUDE_DIR}")

# 2. Check Houdini
elseif (_usd_is_houdini)
    set(USD_INCLUDE_DIR "${USD_ROOT}/toolkit/include")

# 3. Check Maya USD Devkit (0.34.0+)
elseif (DEFINED MAYA_USD_DEVKIT_PATH AND EXISTS "${MAYA_USD_DEVKIT_PATH}/include/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${MAYA_USD_DEVKIT_PATH}/include")
    message(STATUS "[AYON] Detected MayaUSD 0.34.0+ layout — headers in Maya devkit.")
    message(STATUS "       - Found headers at: ${USD_INCLUDE_DIR}")

# 4. Check MayaUSD Split Layout (Linux)
elseif (EXISTS "${USD_ROOT}/../../../devkit/include/pxr/pxr.h")
    get_filename_component(_usd_common_ancestor "${USD_ROOT}/../../.." ABSOLUTE)
    set(USD_INCLUDE_DIR "${_usd_common_ancestor}/devkit/include")
    message(STATUS "[AYON] Detected MayaUSD Split Layout (Linux).")
    message(STATUS "       - Found headers at: ${USD_INCLUDE_DIR}")

# 5. Standard Layouts
elseif (EXISTS "${USD_ROOT}/include/maya-usd/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${USD_ROOT}/include/maya-usd")
    message(STATUS "[AYON] Found Include Dir (Maya USD): ${USD_INCLUDE_DIR}")
elseif (EXISTS "${USD_ROOT}/include/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${USD_ROOT}/include")
    message(STATUS "[AYON] Found Include Dir (Standard): ${USD_INCLUDE_DIR}")
elseif (EXISTS "${USD_ROOT}/../include/pxr/pxr.h")
    get_filename_component(_usd_parent "${USD_ROOT}" DIRECTORY)
    set(USD_INCLUDE_DIR "${_usd_parent}/include")
    message(STATUS "[AYON] Found Include Dir (Parent): ${USD_INCLUDE_DIR}")
elseif (EXISTS "${USD_ROOT}/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${USD_ROOT}")
else()
    message(FATAL_ERROR
        "Could not find USD include directory under ${USD_ROOT}. "
        "Checked toolkit/include, ../../../devkit/include, include/maya-usd, include/, ../include/, and .")
endif()

# =========================================================
# Locate library directory
# =========================================================
if (EXISTS "${USD_ROOT}/dsolib")
    set(USD_LIB_DIR "${USD_ROOT}/dsolib")
    message(STATUS "[AYON] Found Houdini dsolib: ${USD_LIB_DIR}")

elseif (EXISTS "${USD_ROOT}/custom/houdini/dsolib")
    set(USD_LIB_DIR "${USD_ROOT}/custom/houdini/dsolib")
    message(STATUS "[AYON] Found Windows Houdini Libs: ${USD_LIB_DIR}")

# Maya USD: Check lib/usd first for .lib files
elseif (EXISTS "${USD_ROOT}/lib/usd")
    # Check if .lib files exist in lib/usd/
    if (EXISTS "${USD_ROOT}/lib/usd/usd_tf.lib" OR EXISTS "${USD_ROOT}/lib/usd/libusd_tf.lib")
        set(USD_LIB_DIR "${USD_ROOT}/lib/usd")
        message(STATUS "[AYON] Found MayaUSD lib/usd layout (with .lib files): ${USD_LIB_DIR}")
    else()
        # .lib files are in lib/, .dll files in lib/usd/
        set(USD_LIB_DIR "${USD_ROOT}/lib")
        message(STATUS "[AYON] Found MayaUSD lib layout (split .lib/.dll): ${USD_LIB_DIR}")
    endif()

elseif (EXISTS "${USD_ROOT}/lib/maya-usd")
    set(USD_LIB_DIR "${USD_ROOT}/lib/maya-usd")
    message(STATUS "[AYON] Found MayaUSD lib/maya-usd layout: ${USD_LIB_DIR}")

elseif (EXISTS "${USD_ROOT}/lib")
    set(USD_LIB_DIR "${USD_ROOT}/lib")
    message(STATUS "[AYON] Found standard lib layout: ${USD_LIB_DIR}")

elseif (EXISTS "${USD_ROOT}/../dsolib")
    get_filename_component(_usd_parent "${USD_ROOT}" DIRECTORY)
    set(USD_LIB_DIR "${_usd_parent}/dsolib")
    message(STATUS "[AYON] Found parent dsolib: ${USD_LIB_DIR}")

else()
    message(FATAL_ERROR
        "Could not find USD library directory under ${USD_ROOT}. "
        "Checked dsolib, custom/houdini/dsolib, lib/usd, lib/maya-usd and lib.")
endif()

# =========================================================
# Detect library prefix (libusd_* vs libpxr_*)
# =========================================================
set(_usd_prefix "usd")
# Check for .lib (Windows) and .so/.a (Linux/macOS)
if (EXISTS "${USD_LIB_DIR}/libpxr_tf.so" OR EXISTS "${USD_LIB_DIR}/libpxr_tf.a" OR EXISTS "${USD_LIB_DIR}/pxr_tf.lib" OR EXISTS "${USD_LIB_DIR}/libpxr_tf.lib")
    set(_usd_prefix "pxr")
endif()
message(STATUS "[AYON] Detected USD library prefix: ${_usd_prefix}_")
message(STATUS "[AYON] Looking for libraries in: ${USD_LIB_DIR}")

# =========================================================
# Locate core libraries
# =========================================================
message(STATUS "[AYON] Searching for USD libraries...")

# Special case: Maya USD on Windows with only .dll files (no .lib import libs)
if (WIN32 AND EXISTS "${USD_ROOT}/lib/usd_tf.dll")
    message(STATUS "[AYON] Maya USD .dll files detected (no .lib import libs)")
    
    # On Windows, we can link directly against .dll files
    # CMake will handle this by marking them as SHARED libraries
    set(USD_LIB_TF "${USD_ROOT}/lib/usd_tf.dll")
    set(USD_LIB_AR "${USD_ROOT}/lib/usd_ar.dll")
    set(USD_LIB_PLUG "${USD_ROOT}/lib/usd_plug.dll")
    set(USD_LIB_ARCH "${USD_ROOT}/lib/usd_arch.dll")
    message(STATUS "[AYON] Using .dll files directly")

elseif (WIN32 AND EXISTS "${USD_ROOT}/lib/usd/usd_tf.dll")
    message(STATUS "[AYON] Maya USD subdirectory layout: .dll in lib/usd/")
    set(USD_LIB_TF "${USD_ROOT}/lib/usd/usd_tf.dll")
    set(USD_LIB_AR "${USD_ROOT}/lib/usd/usd_ar.dll")
    set(USD_LIB_PLUG "${USD_ROOT}/lib/usd/usd_plug.dll")
    set(USD_LIB_ARCH "${USD_ROOT}/lib/usd/usd_arch.dll")
    message(STATUS "[AYON] Using .dll files from lib/usd/")

else()
    # Standard search for .lib files
    set(_usd_search_paths 
        "${USD_LIB_DIR}"
        "${USD_ROOT}/lib"
        "${USD_ROOT}/lib/usd"
    )
    
    message(STATUS "[AYON] Searching for .lib files in: ${_usd_search_paths}")
    
    find_library(USD_LIB_TF NAMES ${_usd_prefix}_tf lib${_usd_prefix}_tf PATHS ${_usd_search_paths} NO_DEFAULT_PATH)
    find_library(USD_LIB_AR NAMES ${_usd_prefix}_ar lib${_usd_prefix}_ar PATHS ${_usd_search_paths} NO_DEFAULT_PATH)
    find_library(USD_LIB_PLUG NAMES ${_usd_prefix}_plug lib${_usd_prefix}_plug PATHS ${_usd_search_paths} NO_DEFAULT_PATH)
    find_library(USD_LIB_ARCH NAMES ${_usd_prefix}_arch lib${_usd_prefix}_arch PATHS ${_usd_search_paths} NO_DEFAULT_PATH)
endif()

if (NOT USD_LIB_TF OR NOT USD_LIB_AR OR NOT USD_LIB_PLUG OR NOT USD_LIB_ARCH)
    message(FATAL_ERROR
        "Could not find USD core libraries.\n"
        "Checked in: ${USD_LIB_DIR}\n"
        "Looking for: ${_usd_prefix}_tf, ${_usd_prefix}_ar, ${_usd_prefix}_plug, ${_usd_prefix}_arch.\n"
        "USD_LIB_TF: ${USD_LIB_TF}\n"
        "USD_LIB_AR: ${USD_LIB_AR}\n"
        "USD_LIB_PLUG: ${USD_LIB_PLUG}\n"
        "USD_LIB_ARCH: ${USD_LIB_ARCH}")
endif()

# =========================================================
# Create imported targets
# =========================================================
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

# =========================================================
# Export convenience variables
# =========================================================
set(USD_FOUND TRUE)
set(USD_INCLUDE_DIR ${USD_INCLUDE_DIR} CACHE PATH "USD include directory")
set(USD_LIB_DIR ${USD_LIB_DIR} CACHE PATH "USD library directory")
set(USD_IS_HOUDINI ${_usd_is_houdini} CACHE BOOL "Whether the detected USD layout is Houdini-style" FORCE)
set(USD_LIB_PREFIX ${_usd_prefix} CACHE STRING "Detected USD library name prefix" FORCE)

# =========================================================
# Diagnostics
# =========================================================
message(STATUS "USD found: ${USD_INCLUDE_DIR}")
message(STATUS "USD libs: ${USD_LIB_DIR}")
message(STATUS "USD_LIB_TF: ${USD_LIB_TF}")
message(STATUS "USD_LIB_AR: ${USD_LIB_AR}")
message(STATUS "USD_LIB_PLUG: ${USD_LIB_PLUG}")
message(STATUS "USD_LIB_ARCH: ${USD_LIB_ARCH}")
