# Houdini

set(AR_HOUDINI_ROOT $ENV{HFS} CACHE PATH "Houdini install directory")


if (WIN32)
    set(AR_HOUDINI_LIB_DIR ${AR_HOUDINI_ROOT}/custom/houdini/dsolib)
else()
    set(AR_HOUDINI_LIB_DIR ${AR_HOUDINI_ROOT}/dsolib)
endif()


set(AR_HOUDINI_INCLUDE_DIR ${AR_HOUDINI_ROOT}/toolkit/include)

# setting up usd lib dirs so that main cmakeLists can pick it up 
if (WIN32)
    set(AR_PXR_LIB_DIR ${AR_HOUDINI_ROOT}/custom/houdini/dsolib)
    set(AR_PXR_LIB_PREFIX "libpxr_")
else()
    set(AR_PXR_LIB_DIR ${AR_HOUDINI_ROOT}/dsolib)
    set(AR_PXR_LIB_PREFIX "pxr_")
endif()



# Python
if (WIN32)
    if(EXISTS "${AR_HOUDINI_ROOT}/python310")
        set(PyLibName python3.10)
        set(AR_PYTHON_LIB_NUMBER python310)
    else()
        set(PyLibName python3.9)
        set(AR_PYTHON_LIB_NUMBER python39)
    endif()
else()
    if(EXISTS "${AR_HOUDINI_ROOT}/python/bin/python3.10")
        set(PyLibName python3.10)
        set(AR_PYTHON_LIB_NUMBER python310)
    else()
        set(PyLibName python3.9)
        set(AR_PYTHON_LIB_NUMBER python39)
    endif()
endif()


if (WIN32)
    set(AR_PYTHON_LIB_DIR ${AR_HOUDINI_ROOT}/${AR_PYTHON_LIB_NUMBER}/libs)
    #set(AR_PYTHON_LIB_SITEPACKAGES ${AR_HOUDINI_ROOT}/${AR_PYTHON_LIB_NUMBER}/lib/site-packages)
else()
    set(AR_PYTHON_LIB_DIR ${AR_HOUDINI_ROOT}/python/lib)
    #set(AR_PYTHON_LIB_SITEPACKAGES ${AR_PYTHON_LIB_DIR}/${AR_PYTHON_LIB}/site-packages)
endif()


set(AR_PYTHON_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR}/${PyLibName})
set(AR_BOOST_NAMESPACE hboost)
set(AR_BOOST_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR}/${AR_BOOST_NAMESPACE})
add_compile_definitions(HBOOST_ALL_NO_LIB)



if (NOT WIN32)
    # Houdini 20 - Switched to the new C++11 ABI for Linux https://www.sidefx.com/docs/houdini/news/20/platforms.html
    # For Houdini versions that use gcc 9.3, please set this to _GLIBCXX_USE_CXX11_ABI=0
    file(REAL_PATH ${AR_HOUDINI_ROOT} AR_HOUDINI_ROOT_RESOLVED)
    string(FIND ${AR_HOUDINI_ROOT_RESOLVED} "19.5" AR_HOUDINI_ROOT_IS_H195)
    if (${AR_HOUDINI_ROOT_IS_H195} STREQUAL "-1")
        add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)
        target_compile_definitions(AyonCppApi PUBLIC _GLIBCXX_USE_CXX11_ABI=1)
    else()
        add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
        target_compile_definitions(AyonCppApi PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
    endif()
endif()






set(BOOST_LIB_DIR ${AR_HOUDINI_INCLUDE_DIR})
set(AR_PXR_INCLUDE_DIR ${AR_HOUDINI_INCLUDE_DIR})

# Houdini include dir (might shadow some other libarys but thats what we want)
link_directories(${AR_HOUDINI_LIB_DIR})
