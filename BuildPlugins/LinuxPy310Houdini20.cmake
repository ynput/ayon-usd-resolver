# Houdini

set(AR_HOUDINI_ROOT $ENV{HFS} CACHE PATH "Houdini install directory")



set(AR_HOUDINI_LIB_DIR ${AR_HOUDINI_ROOT}/dsolib)

set(AR_HOUDINI_INCLUDE_DIR ${AR_HOUDINI_ROOT}/toolkit/include)

set(AR_PXR_LIB_DIR ${AR_HOUDINI_ROOT}/dsolib)
set(AR_PXR_LIB_PREFIX "pxr_")





set(PyLibName python3.10)
set(AR_PYTHON_LIB_NUMBER python310)




set(AR_PYTHON_LIB_DIR ${AR_HOUDINI_ROOT}/python/lib)
#set(AR_PYTHON_LIB_SITEPACKAGES ${AR_PYTHON_LIB_DIR}/${AR_PYTHON_LIB}/site-packages)



set(AR_PYTHON_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR}/${PyLibName})
set(AR_BOOST_NAMESPACE hboost)
set(AR_BOOST_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR}/${AR_BOOST_NAMESPACE})
add_compile_definitions(HBOOST_ALL_NO_LIB)




add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)
target_compile_definitions(AyonCppApi PUBLIC _GLIBCXX_USE_CXX11_ABI=1)







set(BOOST_LIB_DIR ${AR_HOUDINI_INCLUDE_DIR})
set(AR_PXR_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR})

# Houdini include dir (might shadow some other libarys but thats what we want)
link_directories(${AR_HOUDINI_LIB_DIR})
