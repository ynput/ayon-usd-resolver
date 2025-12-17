# # # # Try to locate a USD installation.
# # # if (DEFINED ENV{AyonUsdRoot})
# # #     set(USD_ROOT $ENV{AyonUsdRoot})
# # # else()
# # #     message(FATAL_ERROR "Environment variable AyonUsdRoot is not defined.")
# # # endif()

# # # find_path(USD_INCLUDE_DIR pxr/pxr.h
# # #     PATHS "${USD_ROOT}/include"
# # # )

# # # find_library(USD_LIB_AR usd_ar
# # #     PATHS "${USD_ROOT}/lib"
# # # )

# # # find_library(USD_LIB_TF usd_tf
# # #     PATHS "${USD_ROOT}/lib"
# # # )

# # # if (NOT USD_INCLUDE_DIR OR NOT USD_LIB_AR OR NOT USD_LIB_TF)
# # #     message(FATAL_ERROR "USD not found in ${USD_ROOT}")
# # # endif()

# # # # Create imported targets for convenience
# # # add_library(USD::tf UNKNOWN IMPORTED)
# # # set_target_properties(USD::tf PROPERTIES
# # #     IMPORTED_LOCATION ${USD_LIB_TF}
# # #     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# # # )

# # # add_library(USD::ar UNKNOWN IMPORTED)
# # # set_target_properties(USD::ar PROPERTIES
# # #     IMPORTED_LOCATION ${USD_LIB_AR}
# # #     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# # # )

# # # set(USD_FOUND TRUE)
# # # Prevent duplicate target creation
# # if (TARGET USD::tf)
# #     set(USD_FOUND TRUE)
# #     message(STATUS "USD targets already defined, skipping redefinition.")
# #     return()
# # endif()

# # if (DEFINED USD_ROOT)
# #     message(STATUS "Using USD root from CMake variable: ${USD_ROOT}")
# # elseif (DEFINED ENV{AyonUsdRoot})
# #     set(USD_ROOT $ENV{AyonUsdRoot})
# #     message(STATUS "Using USD root from environment: ${USD_ROOT}")
# # else()
# #     message(FATAL_ERROR "USD root not specified. Provide -DUSD_ROOT=path/to/USD")
# # endif()

# # # Find USD libs and headers
# # find_path(USD_INCLUDE_DIR pxr/pxr.h PATHS "${USD_ROOT}/include" NO_DEFAULT_PATH)

# # find_library(USD_LIB_TF usd_tf PATHS "${USD_ROOT}/lib" NO_DEFAULT_PATH)
# # find_library(USD_LIB_AR usd_ar PATHS "${USD_ROOT}/lib" NO_DEFAULT_PATH)
# # find_library(USD_LIB_PLUG usd_plug PATHS "${USD_ROOT}/lib" NO_DEFAULT_PATH)
# # find_library(USD_LIB_ARCH usd_arch PATHS "${USD_ROOT}/lib" NO_DEFAULT_PATH)

# # if (NOT USD_LIB_TF OR NOT USD_LIB_AR OR NOT USD_LIB_PLUG OR NOT USD_LIB_ARCH)
# #     message(FATAL_ERROR "Could not find USD core libraries under ${USD_ROOT}/lib")
# # endif()

# # # Create imported targets
# # add_library(USD::tf UNKNOWN IMPORTED)
# # set_target_properties(USD::tf PROPERTIES
# #     IMPORTED_LOCATION ${USD_LIB_TF}
# #     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# # )

# # add_library(USD::ar UNKNOWN IMPORTED)
# # set_target_properties(USD::ar PROPERTIES
# #     IMPORTED_LOCATION ${USD_LIB_AR}
# #     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# # )

# # add_library(USD::plug UNKNOWN IMPORTED)
# # set_target_properties(USD::plug PROPERTIES
# #     IMPORTED_LOCATION ${USD_LIB_PLUG}
# #     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# # )

# # add_library(USD::arch UNKNOWN IMPORTED)
# # set_target_properties(USD::arch PROPERTIES
# #     IMPORTED_LOCATION ${USD_LIB_ARCH}
# #     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# # )

# # set(USD_FOUND TRUE)
# # set(USD_INCLUDE_DIR ${USD_INCLUDE_DIR} CACHE PATH "USD include directory")
# # set(USD_LIB_DIR "${USD_ROOT}/lib" CACHE PATH "USD library directory")

# # message(STATUS "USD found: ${USD_INCLUDE_DIR}")
# # message(STATUS "USD libs: ${USD_LIB_DIR}")
# # message(STATUS "USD_LIB_TF: ${USD_LIB_TF}")
# # message(STATUS "USD_LIB_AR: ${USD_LIB_AR}")
# # message(STATUS "USD_LIB_PLUG: ${USD_LIB_PLUG}")
# # message(STATUS "USD_LIB_ARCH: ${USD_LIB_ARCH}")
# # -----------------------------------------------------------------------------
# # FindUSD.cmake
# # -----------------------------------------------------------------------------
# # Works with:
# #   • Standalone USD SDK (has include/ and lib/)
# #   • Houdini (toolkit/usd + toolkit/include + dsolib)
# # -----------------------------------------------------------------------------

# # -----------------------------------------------------------------------------
# # FindUSD.cmake
# # -----------------------------------------------------------------------------
# # Universal finder for Pixar USD
# # Supports:
# #   • Standalone USD SDK (has include/ and lib/)
# #   • Houdini (toolkit/usd + toolkit/include + dsolib)
# # -----------------------------------------------------------------------------
# # -----------------------------------------------------------------------------
# # FindUSD.cmake
# # -----------------------------------------------------------------------------
# # Universal finder for Pixar USD (Universal Scene Description)
# # Supports:
# #   • Standalone USD SDKs
# #   • Houdini 19–21 (tested with 21.0.512)
# #   • Future DCC-integrated USD layouts
# # -----------------------------------------------------------------------------

# # -----------------------------------------------------------------------------
# # FindUSD.cmake  — AYON custom finder for Pixar USD
# # -----------------------------------------------------------------------------
# # Supports:
# #   • Standalone USD SDK  (libusd_tf.so)
# #   • Houdini 19–21       (libpxr_tf.so, libpxr_ar.so, …)
# # -----------------------------------------------------------------------------

# message(STATUS "[AYON] Using custom FindUSD.cmake from ${CMAKE_CURRENT_LIST_DIR}")

# if (TARGET USD::tf AND TARGET USD::ar)
#     set(USD_FOUND TRUE)
#     message(STATUS "USD targets already defined, skipping redefinition.")
#     return()
# endif()

# # -----------------------------------------------------------------------------
# # USD root detection
# # -----------------------------------------------------------------------------
# if (DEFINED USD_ROOT)
#     message(STATUS "Using USD root from CMake variable: ${USD_ROOT}")
# elseif (DEFINED ENV{AyonUsdRoot})
#     set(USD_ROOT $ENV{AyonUsdRoot})
#     message(STATUS "Using USD root from environment: ${USD_ROOT}")
# else()
#     message(FATAL_ERROR
#         "USD root not specified.\n"
#         "Please provide -DUSD_ROOT=/path/to/USD or set AyonUsdRoot.")
# endif()

# # -----------------------------------------------------------------------------
# # Detect Houdini layout (19–21)
# # -----------------------------------------------------------------------------
# set(_usd_is_houdini FALSE)
# if (EXISTS "${USD_ROOT}/dsolib" AND EXISTS "${USD_ROOT}/toolkit/include/pxr/pxr.h")
#     set(_usd_is_houdini TRUE)
#     message(STATUS "[AYON] Detected Houdini USD layout at ${USD_ROOT}")
# endif()

# # -----------------------------------------------------------------------------
# # Locate include directory
# # -----------------------------------------------------------------------------
# if (_usd_is_houdini)
#     set(USD_INCLUDE_DIR "${USD_ROOT}/toolkit/include")
# elseif (EXISTS "${USD_ROOT}/include/pxr/pxr.h")
#     set(USD_INCLUDE_DIR "${USD_ROOT}/include")
# elseif (EXISTS "${USD_ROOT}/../include/pxr/pxr.h")
#     get_filename_component(_usd_parent "${USD_ROOT}" DIRECTORY)
#     set(USD_INCLUDE_DIR "${_usd_parent}/include")
# elseif (EXISTS "${USD_ROOT}/pxr/pxr.h")
#     set(USD_INCLUDE_DIR "${USD_ROOT}")
# else()
#     message(FATAL_ERROR
#         "Could not find USD include directory under ${USD_ROOT}. "
#         "Checked toolkit/include, include/, ../include/, and .")
# endif()

# # -----------------------------------------------------------------------------
# # Locate library directory
# # -----------------------------------------------------------------------------
# if (EXISTS "${USD_ROOT}/dsolib")
#     set(USD_LIB_DIR "${USD_ROOT}/dsolib")
# elseif (EXISTS "${USD_ROOT}/lib")
#     set(USD_LIB_DIR "${USD_ROOT}/lib")
# elseif (EXISTS "${USD_ROOT}/../dsolib")
#     get_filename_component(_usd_parent "${USD_ROOT}" DIRECTORY)
#     set(USD_LIB_DIR "${_usd_parent}/dsolib")
# else()
#     message(FATAL_ERROR
#         "Could not find USD library directory under ${USD_ROOT}. "
#         "Checked dsolib and lib.")
# endif()

# # -----------------------------------------------------------------------------
# # Detect library prefix (libusd_* vs libpxr_*)
# # -----------------------------------------------------------------------------
# set(_usd_prefix "usd")
# if (EXISTS "${USD_LIB_DIR}/libpxr_tf.so" OR EXISTS "${USD_LIB_DIR}/libpxr_tf.a")
#     set(_usd_prefix "pxr")
# endif()
# message(STATUS "[AYON] Detected USD library prefix: ${_usd_prefix}_")

# # -----------------------------------------------------------------------------
# # Locate core libraries
# # -----------------------------------------------------------------------------
# find_library(USD_LIB_TF ${_usd_prefix}_tf PATHS "${USD_LIB_DIR}" NO_DEFAULT_PATH)
# find_library(USD_LIB_AR ${_usd_prefix}_ar PATHS "${USD_LIB_DIR}" NO_DEFAULT_PATH)
# find_library(USD_LIB_PLUG ${_usd_prefix}_plug PATHS "${USD_LIB_DIR}" NO_DEFAULT_PATH)
# find_library(USD_LIB_ARCH ${_usd_prefix}_arch PATHS "${USD_LIB_DIR}" NO_DEFAULT_PATH)

# if (NOT USD_LIB_TF OR NOT USD_LIB_AR OR NOT USD_LIB_PLUG OR NOT USD_LIB_ARCH)
#     message(FATAL_ERROR
#         "Could not find USD core libraries in ${USD_LIB_DIR}. "
#         "Checked for ${_usd_prefix}_tf, ${_usd_prefix}_ar, ${_usd_prefix}_plug, ${_usd_prefix}_arch.")
# endif()

# # -----------------------------------------------------------------------------
# # Create imported targets
# # -----------------------------------------------------------------------------
# add_library(USD::tf UNKNOWN IMPORTED)
# set_target_properties(USD::tf PROPERTIES
#     IMPORTED_LOCATION ${USD_LIB_TF}
#     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# )

# add_library(USD::ar UNKNOWN IMPORTED)
# set_target_properties(USD::ar PROPERTIES
#     IMPORTED_LOCATION ${USD_LIB_AR}
#     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# )

# add_library(USD::plug UNKNOWN IMPORTED)
# set_target_properties(USD::plug PROPERTIES
#     IMPORTED_LOCATION ${USD_LIB_PLUG}
#     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# )

# add_library(USD::arch UNKNOWN IMPORTED)
# set_target_properties(USD::arch PROPERTIES
#     IMPORTED_LOCATION ${USD_LIB_ARCH}
#     INTERFACE_INCLUDE_DIRECTORIES ${USD_INCLUDE_DIR}
# )

# # -----------------------------------------------------------------------------
# # Export convenience variables
# # -----------------------------------------------------------------------------
# set(USD_FOUND TRUE)
# set(USD_INCLUDE_DIR ${USD_INCLUDE_DIR} CACHE PATH "USD include directory")
# set(USD_LIB_DIR ${USD_LIB_DIR} CACHE PATH "USD library directory")

# # -----------------------------------------------------------------------------
# # Diagnostics
# # -----------------------------------------------------------------------------
# message(STATUS "USD found: ${USD_INCLUDE_DIR}")
# message(STATUS "USD libs: ${USD_LIB_DIR}")
# message(STATUS "USD_LIB_TF: ${USD_LIB_TF}")
# message(STATUS "USD_LIB_AR: ${USD_LIB_AR}")
# message(STATUS "USD_LIB_PLUG: ${USD_LIB_PLUG}")
# message(STATUS "USD_LIB_ARCH: ${USD_LIB_ARCH}")
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
if (_usd_is_houdini)
    set(USD_INCLUDE_DIR "${USD_ROOT}/toolkit/include")
# --- Check for Maya-USD specific include layout ---
elseif (EXISTS "${USD_ROOT}/include/maya-usd/pxr/pxr.h")
    set(USD_INCLUDE_DIR "${USD_ROOT}/include/maya-usd")
    message(STATUS "[AYON] Found Include Dir (Maya USD): ${USD_INCLUDE_DIR}")
# --------------------------------------------------
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
        "Checked toolkit/include, include/maya-usd, include/, ../include/, and .")
endif()

# -----------------------------------------------------------------------------
# Locate library directory
# -----------------------------------------------------------------------------
if (EXISTS "${USD_ROOT}/dsolib")
    set(USD_LIB_DIR "${USD_ROOT}/dsolib")
# --- FIX: Check for Windows Houdini SDK lib path ---
elseif (EXISTS "${USD_ROOT}/custom/houdini/dsolib")
    set(USD_LIB_DIR "${USD_ROOT}/custom/houdini/dsolib")
    message(STATUS "[AYON] Found Windows Houdini Libs: ${USD_LIB_DIR}")
# ---------------------------------------------------
# --- Check for Maya-USD specific lib layout ---
elseif (EXISTS "${USD_ROOT}/lib/maya-usd")
    set(USD_LIB_DIR "${USD_ROOT}/lib/maya-usd")
# ----------------------------------------------
elseif (EXISTS "${USD_ROOT}/lib")
    set(USD_LIB_DIR "${USD_ROOT}/lib")
elseif (EXISTS "${USD_ROOT}/../dsolib")
    get_filename_component(_usd_parent "${USD_ROOT}" DIRECTORY)
    set(USD_LIB_DIR "${_usd_parent}/dsolib")
else()
    message(FATAL_ERROR
        "Could not find USD library directory under ${USD_ROOT}. "
        "Checked dsolib, custom/houdini/dsolib, lib/maya-usd and lib.")
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
