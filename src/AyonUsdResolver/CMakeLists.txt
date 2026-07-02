set(CORE_LIB_TARGET AyonUsdResolver)
set(PYTHON_MOD_TARGET AyonUsdResolver_python)

if(WIN32)
    set(AR_ARCH_LIB_SUFFIX "dll")
else()
    set(AR_ARCH_LIB_SUFFIX "so")
endif()

include(AyonResolverDeps)

# Optional memcached support via libmemcached
# Users can install libmemcached via: apt-get install libmemcached-dev (Linux) or brew install libmemcached (macOS)
option(AYON_MEMCACHED_STATIC_LINK "Link libmemcached statically (requires static build of libmemcached)" OFF)

find_package(libmemcached CONFIG QUIET)
find_package(PkgConfig QUIET)

if(TARGET libmemcached::libmemcached)
    set(LIBMEMCACHED_FOUND TRUE)
    set(AYON_LIBMEMCACHED_TARGET libmemcached::libmemcached)
endif()

if(NOT LIBMEMCACHED_FOUND AND PkgConfig_FOUND)
    if(AYON_MEMCACHED_STATIC_LINK)
        set(_AYON_ORIG_PKG_CONFIG_ARGN "${PKG_CONFIG_ARGN}")
        set(PKG_CONFIG_ARGN ${PKG_CONFIG_ARGN} --static)
    endif()

    pkg_check_modules(LIBMEMCACHED QUIET IMPORTED_TARGET libmemcached)

    if(AYON_MEMCACHED_STATIC_LINK)
        set(PKG_CONFIG_ARGN "${_AYON_ORIG_PKG_CONFIG_ARGN}")
    endif()
endif()

if(NOT LIBMEMCACHED_FOUND)
    find_path(AYON_LIBMEMCACHED_INCLUDE_DIR
        NAMES
            libmemcached-1.0/memcached.h
            libmemcached/memcached.h
            libmemcached.h
        PATH_SUFFIXES include
    )

    find_library(AYON_LIBMEMCACHED_LIBRARY
        NAMES
            memcached
            libmemcached
            libmemcachedawesome
        PATH_SUFFIXES lib lib64
    )

    find_library(AYON_LIBHASHKIT_LIBRARY
        NAMES
            hashkit
            libhashkit
        PATH_SUFFIXES lib lib64
    )

    if(AYON_LIBMEMCACHED_INCLUDE_DIR AND AYON_LIBMEMCACHED_LIBRARY)
        if(NOT TARGET AyonLibmemcached)
            add_library(AyonLibmemcached UNKNOWN IMPORTED)
            set_target_properties(AyonLibmemcached PROPERTIES
                IMPORTED_LOCATION "${AYON_LIBMEMCACHED_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${AYON_LIBMEMCACHED_INCLUDE_DIR}"
            )
            if(AYON_LIBHASHKIT_LIBRARY)
                set_property(TARGET AyonLibmemcached APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES "${AYON_LIBHASHKIT_LIBRARY}"
                )
            endif()
            if(WIN32)
                set_property(TARGET AyonLibmemcached APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES ws2_32
                )
            endif()
        endif()

        set(LIBMEMCACHED_FOUND TRUE)
        set(AYON_LIBMEMCACHED_TARGET AyonLibmemcached)
    endif()
endif()

if(LIBMEMCACHED_FOUND)
    set(AYON_LIBMEMCACHED_AVAILABLE TRUE)
    if(NOT AYON_LIBMEMCACHED_TARGET)
        set(AYON_LIBMEMCACHED_TARGET PkgConfig::LIBMEMCACHED)
    endif()
    if(AYON_LIBMEMCACHED_TARGET STREQUAL "libmemcached::libmemcached")
        message(STATUS "[AYON] libmemcached found via CMake package config - memcached support ENABLED")
    elseif(AYON_LIBMEMCACHED_TARGET STREQUAL "PkgConfig::LIBMEMCACHED")
        message(STATUS "[AYON] libmemcached found via pkg-config - memcached support ENABLED")
    else()
        message(STATUS "[AYON] libmemcached found via CMake library search - memcached support ENABLED")
    endif()
else()
    set(AYON_LIBMEMCACHED_AVAILABLE FALSE)
    message(STATUS "[AYON] libmemcached NOT found - memcached support DISABLED")
    if(PkgConfig_FOUND)
        message(STATUS "       To enable memcached: install libmemcached-dev (Linux) or libmemcached (macOS/Homebrew)")
    else()
        message(STATUS "       pkg-config was not found; CMake also searched standard prefixes and CMAKE_PREFIX_PATH")
    endif()
endif()


set(RESOLVER_SRCS
    resolver.cpp
    resolverContext.cpp
    Wrappers_Tokens/resolverTokens.cpp
    helpers/resolutionFunctions.cpp
    cache/assetIdentifierDef.cpp
    cache/resolverContextCache.cpp
    cache/memcachedHandler.cpp
    helpers/ayonApiGet.cpp
)

add_library(${CORE_LIB_TARGET} SHARED ${RESOLVER_SRCS})
target_link_libraries(${CORE_LIB_TARGET} PRIVATE AyonResolver::DccDeps)

# Link memcached only if available
if(AYON_LIBMEMCACHED_AVAILABLE)
    target_link_libraries(${CORE_LIB_TARGET} PRIVATE ${AYON_LIBMEMCACHED_TARGET})
    target_compile_definitions(${CORE_LIB_TARGET} PRIVATE AYON_LIBMEMCACHED_AVAILABLE=1 WIN32_LEAN_AND_MEAN)
    if(AYON_MEMCACHED_STATIC_LINK AND WIN32)
        # Tell libmemcached headers not to use __declspec(dllimport)
        target_compile_definitions(${CORE_LIB_TARGET} PRIVATE LIBMEMCACHED_STATIC)
    endif()
else()
    target_compile_definitions(${CORE_LIB_TARGET} PRIVATE AYON_LIBMEMCACHED_AVAILABLE=0)
endif()

target_include_directories(${CORE_LIB_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(${CORE_LIB_TARGET} PROPERTIES PREFIX "" OUTPUT_NAME "ayonUsdResolver")

if(UNIX)
    target_link_options(${CORE_LIB_TARGET} PRIVATE "LINKER:--no-undefined")
endif()

if(WIN32)
    target_compile_definitions(${CORE_LIB_TARGET} PRIVATE AR_AYONUSDRESOLVER_EXPORTS)
endif()

if(MSVC)
    target_compile_options(${CORE_LIB_TARGET} PRIVATE /Zc:inline-)
    target_link_options(${CORE_LIB_TARGET} PRIVATE /OPT:NOREF)
endif()

add_library(${PYTHON_MOD_TARGET} MODULE
    Wrappers_Tokens/wrapResolverContext.cpp
    Wrappers_Tokens/wrapResolver.cpp
    Wrappers_Tokens/wrapResolverTokens.cpp
    pluginData/moduleDeps.cpp
    pluginData/module.cpp
)

target_link_libraries(${PYTHON_MOD_TARGET}
    PUBLIC
        ${CORE_LIB_TARGET}
        AyonResolver::DccDeps
        AyonResolver::PythonDeps
)
target_include_directories(${PYTHON_MOD_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(${PYTHON_MOD_TARGET} PRIVATE AR_AYONUSDRESOLVER_EXPORTS)

if(UNIX)
    target_link_options(${PYTHON_MOD_TARGET} PRIVATE "LINKER:--no-undefined")
endif()

if(MSVC)
    target_compile_options(${PYTHON_MOD_TARGET} PRIVATE /Zc:inline-)
    target_link_options(${PYTHON_MOD_TARGET} PRIVATE /OPT:NOREF)
endif()

set(PLUGIN_BASE_DIR "ayonUsdResolver")
set(AR_AYONUSDRESOLVER_USD_CXX_CLASS_NAME "AyonUsdResolver")
set(AR_AYONUSDRESOLVER_USD_PLUGIN_NAME "ayonUsdResolver")
set(AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME "usdAssetResolver.AyonUsdResolver")
set(AR_AYONUSDRESOLVER_TARGET_LIB "ayonUsdResolver")

set(AYON_USD_MODULE_DEFINITIONS
    MFB_ALT_PACKAGE_NAME=AyonUsdResolver
    MFB_PACKAGE_NAME=ayonUsdResolver
    MFB_PACKAGE_MODULE=usdAssetResolver.AyonUsdResolver
    AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME=usdAssetResolver.AyonUsdResolver
)

target_compile_definitions(${PYTHON_MOD_TARGET} PRIVATE ${AYON_USD_MODULE_DEFINITIONS})

if(WIN32)
    set_target_properties(${PYTHON_MOD_TARGET} PROPERTIES
        PREFIX "" SUFFIX ".pyd" OUTPUT_NAME "_ayonUsdResolver")
else()
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
