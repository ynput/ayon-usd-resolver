cmake_minimum_required(VERSION 3.20)

project(AyonUsdResolverProject LANGUAGES CXX)

# --------------------------------------------------------
# Make sure our custom FindUSD.cmake overrides Houdini's
# --------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)
set(CMAKE_FIND_PACKAGE_PREFER_MODULE TRUE)

message(STATUS "[AYON] Custom CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}")

# ----------------------------------------
# General settings
# ----------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Where to look for custom modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ----------------------------------------
# Find USD
# ----------------------------------------
find_package(USD REQUIRED)

# ----------------------------------------
# AyonCppApi (local subproject)
# ----------------------------------------
add_subdirectory(ext/ayon-cpp-api)

# ----------------------------------------
# Build the USD resolver
# ----------------------------------------
add_subdirectory(src/AyonUsdResolver)

# ----------------------------------------
# Packaging setup (optional)
# ----------------------------------------
include(CPack)
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "AyonUsdResolver")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
