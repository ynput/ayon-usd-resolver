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

# Houdini 19.x ships USD against the old libstdc++ ABI. Apply the ABI define
# before subprojects are added so AyonCppApi and the resolver are compiled
# consistently and do not disagree on mangled std::string symbols.
if(BUILD_TARGET STREQUAL "houdini" AND NOT WIN32 AND DEFINED USD_ROOT)
    string(REGEX MATCH "hfs([0-9]+\\.[0-9]+)" AYON_HOU_VER_MATCH "${USD_ROOT}")
    set(AYON_HOU_VER "${CMAKE_MATCH_1}")
    if(AYON_HOU_VER AND AYON_HOU_VER VERSION_LESS "20.0")
        message(STATUS
            "[AYON] Houdini ${AYON_HOU_VER} detected before subprojects. "
            "Applying _GLIBCXX_USE_CXX11_ABI=0 globally.")
        add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
    endif()
endif()

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
