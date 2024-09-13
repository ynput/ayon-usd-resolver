if (WIN32)
    add_compile_definitions(AR_AYONUSDRESOLVER_EXPORTS)
endif()
### Targets ###
add_library(${AR_AYONUSDRESOLVER_TARGET_LIB}
    SHARED
        cache/assetIdentDef.cpp
        codes/debugCodes.cpp

        resolver.cpp
        resolverContext.cpp
        Wrappers_Tokens/resolverTokens.cpp 
		
        cache/resolverContextCache.cpp
        helpers/resolutionFunctions.cpp 

)

target_compile_definitions(${AR_AYONUSDRESOLVER_TARGET_LIB}
    PRIVATE
        # USD Plugin Internal Namings
        MFB_PACKAGE_NAME=${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}
        MFB_PACKAGE_MODULE=${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME}
        AR_AYONUSDRESOLVER_USD_PLUGIN_NAME=${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}
        AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME=${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME}
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
target_link_libraries(${AR_AYONUSDRESOLVER_TARGET_LIB} AyonCppDevToolsLib)

# Install
configure_file(plugInfo.json.in plugInfo.json)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json DESTINATION ${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}/resources)
install(TARGETS ${AR_AYONUSDRESOLVER_TARGET_LIB} DESTINATION ${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}/lib)


add_library(${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    SHARED
        pluginData/module.cpp
        pluginData/moduleDeps.cpp
        Wrappers_Tokens/wrapResolver.cpp
        Wrappers_Tokens/wrapResolverContext.cpp
        Wrappers_Tokens/wrapResolverTokens.cpp 
        Wrappers_Tokens/resolverTokens.cpp 

)
add_dependencies(${AR_AYONUSDRESOLVER_TARGET_PYTHON} ${AR_AYONUSDRESOLVER_TARGET_LIB})
set_boost_namespace(${AR_AYONUSDRESOLVER_TARGET_PYTHON})
# Libs
target_link_libraries(${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    ${AR_AYONUSDRESOLVER_TARGET_LIB}
    ${AR_BOOST_PYTHON_LIB}
)

target_link_libraries(${AR_AYONUSDRESOLVER_TARGET_PYTHON} AyonCppApi)
target_link_libraries(${AR_AYONUSDRESOLVER_TARGET_PYTHON} AyonCppDevToolsLib)

# Headers
target_include_directories(${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    PUBLIC
        ${AR_BOOST_INCLUDE_DIR}
        ${AR_PYTHON_INCLUDE_DIR}
        ${AR_PXR_INCLUDE_DIR}
)
# Remove default "lib" prefix
set_target_properties(${AR_AYONUSDRESOLVER_TARGET_PYTHON} PROPERTIES PREFIX "")
if (WIN32)
    set_target_properties(${AR_AYONUSDRESOLVER_TARGET_PYTHON} PROPERTIES SUFFIX ".pyd")
endif()

target_compile_definitions(${AR_AYONUSDRESOLVER_TARGET_PYTHON}
    PRIVATE
        # USD Plugin Internal Namings
        MFB_PACKAGE_NAME=${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}
        MFB_PACKAGE_MODULE=${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_NAME}
        AR_AYONUSDRESOLVER_USD_PLUGIN_NAME=${AR_AYONUSDRESOLVER_USD_PLUGIN_NAME}
        AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME=${AR_AYONUSDRESOLVER_USD_PYTHON_MODULE_FULLNAME}
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
