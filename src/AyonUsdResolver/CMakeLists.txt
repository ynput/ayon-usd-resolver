### CPP Settings ###
if (WIN32)
    add_compile_definitions(AR_AYONUSDRESOLVER_EXPORTS)
endif()

### Targets ###
add_library(${AR_AYONUSDRESOLVER_TARGET_LIB}
    SHARED
        debugCodes.cpp
        resolver.cpp
        resolverContext.cpp
        resolverTokens.cpp 
        resolverContextCache.cpp
        reseutionFunctions.cpp 
)

set_boost_namespace(${AR_AYONUSDRESOLVER_TARGET_LIB})
# Libs
target_link_libraries(${AR_AYONUSDRESOLVER_TARGET_LIB}
    ${AR_PXR_LIB_PREFIX}arch
    ${AR_PXR_LIB_PREFIX}tf
    ${AR_PXR_LIB_PREFIX}gf
    ${AR_PXR_LIB_PREFIX}vt
    ${AR_PXR_LIB_PREFIX}ar
    ${AR_PXR_LIB_PREFIX}sdf
    ${AR_BOOST_PYTHON_LIB}
)
# Headers
target_include_directories(${AR_AYONUSDRESOLVER_TARGET_LIB}
    PUBLIC
    ${AR_BOOST_INCLUDE_DIR}
    ${AR_PYTHON_INCLUDE_DIR}
    ${AR_PXR_INCLUDE_DIR}
)
# Props
# Remove default "lib" prefix
set_target_properties(${AR_AYONUSDRESOLVER_TARGET_LIB} PROPERTIES PREFIX "")
# Preprocessor Defines (Same as #define)
target_compile_definitions(${AR_AYONUSDRESOLVER_TARGET_LIB}
    PRIVATE
        # USD Plugin Internal Namings
        MFB_PACKAGE_NAME=${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}
)

target_link_libraries(${AR_AYONUSDRESOLVER_TARGET_LIB} AyonCppApi)



# Install
configure_file(plugInfo.json.in plugInfo.json)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json DESTINATION ${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}/resources)
install(TARGETS ${AR_AYONUSDRESOLVER_TARGET_LIB} DESTINATION ${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}/lib)

add_library(${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    SHARED
        module.cpp
        moduleDeps.cpp
        resolverTokens.cpp
        wrapResolver.cpp
        wrapResolverContext.cpp
        wrapResolverTokens.cpp
)
add_dependencies(${AR_AYONUSDRESOLVER_TARGET_PYTHON} ${AR_AYONUSDRESOLVER_TARGET_LIB})
set_boost_namespace(${AR_AYONUSDRESOLVER_TARGET_PYTHON})
# Libs
target_link_libraries(${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    ${AR_AYONUSDRESOLVER_TARGET_LIB}
    ${AR_BOOST_PYTHON_LIB}
)
# Headers
target_include_directories(${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    PUBLIC
        ${AR_BOOST_INCLUDE_DIR}
        ${AR_PYTHON_INCLUDE_DIR}
        ${AR_PXR_INCLUDE_DIR}
)
# Props
# Remove default "lib" prefix
set_target_properties(${AR_AYONUSDRESOLVER_TARGET_PYTHON} PROPERTIES PREFIX "")
if (WIN32)
    set_target_properties(${AR_AYONUSDRESOLVER_TARGET_PYTHON} PROPERTIES SUFFIX ".pyd")
endif()
# Preprocessor Defines (Same as #define)
target_compile_definitions(${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    PRIVATE
        # USD Plugin Internal Namings
        MFB_PACKAGE_NAME=${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}
        MFB_PACKAGE_MODULE=${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME}
        AR_AYONUSDRESOLVER_USD_PLUGIN_NAME=${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}
        AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME=${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME}
        # AR_AYONUSDRESOLVER_USD_PYTHON_EXPOSE_MODULE_NAME=${AR_AYONUSDRESOLVER_USD_PYTHON_EXPOSE_MODULE_NAME}
)
# Install
install (
    FILES __init__.py
    DESTINATION ${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}/lib/python/${AR_RESOLVER_USD_PYTHON_MODULE_NAME}/${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME}
)

install(
    TARGETS ${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    DESTINATION ${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}/lib/python/${AR_RESOLVER_USD_PYTHON_MODULE_NAME}/${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME}
)
