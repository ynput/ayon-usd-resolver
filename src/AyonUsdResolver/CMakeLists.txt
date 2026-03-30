set(CORE_LIB_TARGET AyonUsdResolver)
set(PYTHON_MOD_TARGET AyonUsdResolver_python)

if(WIN32)
    set(AR_ARCH_LIB_SUFFIX "dll")
else()
    set(AR_ARCH_LIB_SUFFIX "so")
endif()

include(AyonResolverDeps)

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
target_link_libraries(${CORE_LIB_TARGET} PRIVATE AyonResolver::DccDeps)
target_include_directories(${CORE_LIB_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(${CORE_LIB_TARGET} PROPERTIES PREFIX "" OUTPUT_NAME "ayonUsdResolver")

if(UNIX AND NOT BUILD_TARGET STREQUAL "maya")
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
    Wrappers_Tokens/resolverTokens.cpp
    pluginData/module.cpp
)

target_link_libraries(${PYTHON_MOD_TARGET}
    PUBLIC
        ${CORE_LIB_TARGET}
        AyonResolver::DccDeps
)
target_include_directories(${PYTHON_MOD_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(${PYTHON_MOD_TARGET} PRIVATE AR_AYONUSDRESOLVER_EXPORTS)

if(UNIX AND NOT BUILD_TARGET STREQUAL "maya")
    target_link_options(${PYTHON_MOD_TARGET} PRIVATE "LINKER:--no-undefined")
endif()

if(MSVC)
    target_compile_options(${PYTHON_MOD_TARGET} PRIVATE /Zc:inline-)
    target_link_options(${PYTHON_MOD_TARGET} PRIVATE /OPT:NOREF)
endif()

set(PLUGIN_BASE_DIR "ayonUsdResolver")
set(AR_AYONUSDRESOLVER_USD_CXX_CLASS_NAME "AyonUsdResolver")
set(AR_AYONUSDRESOLVER_USD_PLUGIN_NAME "ayonUsdResolver")
set(AR_AYONUSDRESOLVER_TARGET_LIB "ayonUsdResolver")

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
