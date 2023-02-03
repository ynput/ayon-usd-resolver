set(PLUGIN_NAME YON_AssetResolverPlugin)

# .dll on Windows, .so on Linux
add_library(${PLUGIN_NAME} 
	SHARED
)

# --------- linker ---------------
target_link_directories(${PLUGIN_NAME} 
	PUBLIC
		${USD_LIBRARY_DIR}
		${PYTHON_LIBRARIES_DIR}
)

target_link_libraries(${PLUGIN_NAME}
    usd_ar
	usd_arch
    usd_sdf
	usd_tf
	usd_js
)

# VS 2019 specific linking python library
if(MSVC)
	target_link_libraries(${PLUGIN_NAME}
		${PYTHON_LIBRARY}
	)
endif()
#--------------------------------

# external c++ header files
target_include_directories(${PLUGIN_NAME}
	PRIVATE
		${PYTHON_INCLUDE_DIR}
		${Boost_INCLUDE_DIR}
		${USD_INCLUDE_DIR}
)

# internal c++ files
target_sources(${PLUGIN_NAME}
	PUBLIC		
		YON_AssetResolver.h		
		YON_AssetResolvingProvider_REST.h
		YON_AssetResolveResult.h
		YON_AssetResolvingProvider.h
		Logging/YON_Logging.h
	PRIVATE
		YON_AssetResolver.cpp
		YON_AssetResolvingProvider_REST.cpp
		YON_AssetResolveResult.cpp
		Logging/YON_Logging.cpp
)



# -------- .so vs .dll export in plugInfo.json ---------------
set(EXPORTED_LIB_NAME "${PLUGIN_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
if(UNIX)
	set(EXPORTED_LIB_NAME "lib${EXPORTED_LIB_NAME}")
endif()
set(PLUG_INFO_LIBRARY_PATH "./${EXPORTED_LIB_NAME}")

configure_file(
        plugInfo.json
        ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
)
# -----------------------------------------------------------


# instalation
set(install_path "${PLUGIN_NAME}")
install(
    TARGETS
        ${PLUGIN_NAME}
	DESTINATION
		${install_path}
)



install(
    FILES
		${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION
		${install_path}/resources
)
