include_guard(GLOBAL)

find_package(Threads REQUIRED)
include(AyonResolverOpenSSL)

set(AYON_RESOLVER_DCC_DEPS_TARGET AyonResolverDccDeps)
if(NOT TARGET ${AYON_RESOLVER_DCC_DEPS_TARGET})
    add_library(${AYON_RESOLVER_DCC_DEPS_TARGET} INTERFACE)
    add_library(AyonResolver::DccDeps ALIAS ${AYON_RESOLVER_DCC_DEPS_TARGET})
endif()

set(AYON_RESOLVER_DCC_INCLUDE_DIRS "")
set(AYON_RESOLVER_DCC_LINK_LIBS "")
set(AYON_RESOLVER_DCC_COMPILE_DEFINITIONS "")

if(BUILD_TARGET STREQUAL "maya")
    include(AyonResolverMaya)
elseif(USD_IS_HOUDINI)
    include(AyonResolverHoudini)
else()
    include(AyonResolverUsd)
endif()

if(Python_FOUND AND NOT TARGET Python::Python AND Python_LIBRARIES)
    add_library(Python::Python UNKNOWN IMPORTED)
    set_target_properties(Python::Python PROPERTIES
        IMPORTED_LOCATION "${Python_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${Python_INCLUDE_DIRS}"
    )
endif()

set(AYON_RESOLVER_EXTRA_USD_LIBS "")
if(USD_IS_HOUDINI OR BUILD_TARGET STREQUAL "maya")
    set(_ayon_usd_prefix "${USD_LIB_PREFIX}")
    if(NOT _ayon_usd_prefix)
        set(_ayon_usd_prefix "usd")
    endif()

    foreach(_libname sdf vt pcp)
        find_library(_ayon_usd_lib_found
            NAMES ${_ayon_usd_prefix}_${_libname} lib${_ayon_usd_prefix}_${_libname}
            PATHS "${USD_LIB_DIR}"
            NO_DEFAULT_PATH
        )
        if(_ayon_usd_lib_found)
            list(APPEND AYON_RESOLVER_EXTRA_USD_LIBS "${_ayon_usd_lib_found}")
            message(STATUS "[AYON] Found USD library: ${_libname} = ${_ayon_usd_lib_found}")
            unset(_ayon_usd_lib_found CACHE)
        endif()
    endforeach()
endif()

target_compile_definitions(${AYON_RESOLVER_DCC_DEPS_TARGET}
    INTERFACE
        PXR_SET_INTERNAL_NAMESPACE
        HBOOST_ALL_NO_LIB
        ${AYON_RESOLVER_DCC_COMPILE_DEFINITIONS}
)

if(MSVC)
    target_compile_definitions(${AYON_RESOLVER_DCC_DEPS_TARGET}
        INTERFACE
            _ITERATOR_DEBUG_LEVEL=0
            TBB_USE_ASSERT=0
            TBB_USE_THREADING_TOOLS=0
            __TBB_NO_IMPLICIT_LINKAGE
    )
endif()

target_include_directories(${AYON_RESOLVER_DCC_DEPS_TARGET}
    INTERFACE
        "${USD_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/src/AyonCppApi"
        "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src"
        "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/ayon-cpp-dev-tools/src/lib"
        "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/cpp-httplib"
        "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/json/include"
        "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/spdlog/include"
        ${AYON_RESOLVER_DCC_INCLUDE_DIRS}
)

target_link_libraries(${AYON_RESOLVER_DCC_DEPS_TARGET}
    INTERFACE
        Threads::Threads
        AyonCppApi
        AyonCppDevToolsLib
        OpenSSL::SSL
        OpenSSL::Crypto
        USD::ar
        USD::tf
        USD::plug
        USD::arch
        ${AYON_RESOLVER_EXTRA_USD_LIBS}
        ${AYON_RESOLVER_DCC_LINK_LIBS}
)

if(TARGET Python::Python)
    target_link_libraries(${AYON_RESOLVER_DCC_DEPS_TARGET} INTERFACE Python::Python)
endif()

message(STATUS "[AYON] Final AR_BOOST_INCLUDE_DIR: ${AR_BOOST_INCLUDE_DIR}")
message(STATUS "[AYON] Final Python_INCLUDE_DIRS: ${Python_INCLUDE_DIRS}")
