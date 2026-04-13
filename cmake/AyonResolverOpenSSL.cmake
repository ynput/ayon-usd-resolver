include_guard(GLOBAL)

if(NOT DEFINED OPENSSL_ROOT_DIR OR NOT OPENSSL_ROOT_DIR)
    if(WIN32)
        set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/opensslW")
    elseif(USE_OPENSSL3)
        set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/openssl3L")
    else()
        set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/ext/ayon-cpp-api/ext/openssl1L")
    endif()
endif()

message(STATUS "[AYON] Resolver OpenSSL root: ${OPENSSL_ROOT_DIR}")
find_package(OpenSSL REQUIRED)
