# Variable's that need to be set by the plugin ( the env variables will be checked by the Main CMakeLists.txt and an error will occur if one or more are missing )
# AR_PXR_INCLUDE_DIR
# AR_PXR_LIB_DIR
# AR_PYTHON_LIB_NUMBER
# AR_BOOST_INCLUDE_DIR
# BOOST_LIB_DIR
# AR_PYTHON_LIB_DIR
# AR_PYTHON_INCLUDE_DIR
# AR_PXR_LIB_PREFIX


# in the case off AyonUsd we need to set project before finding the Python package or we end up in an variable change dead lock
project(${AR_PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(AR_AYONUSD_ROOT $ENV{AyonUsdRoot} CACHE PATH "Ayon Usd install directory")
if (NOT DEFINED ENV{AyonUsdRoot})
  message(FATAL_ERROR "AyonUsdRoot Env Variable is not defined. But BuildPlugin AyonUsd needs it to function.") 
endif()

# Set up AyonUsd
set(AR_PXR_LIB_DIR ${AR_AYONUSD_ROOT}/lib)
set(AR_PXR_LIB_PREFIX "usd_")
set(AR_PXR_INCLUDE_DIR ${AR_AYONUSD_ROOT}/include)

# Set up Python

# We are currently hard-coding the relative portion of the Python directories. In some cases, your internal folder structure might differ. Then, you have to change AR_PYTHON_LIB_DIR and AR_PYTHON_INCLUDE_DIR, respectively. 

find_package(Python COMPONENTS Interpreter)
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import sys; print(sys.version[0:3])"
    OUTPUT_VARIABLE Python_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT ${Python_VERSION} STREQUAL 3.9) # Check if System Installed Python version is 3.9
  message(FATAL_ERROR "Python version: ${Python_VERSION}")
endif()
find_package(Python COMPONENTS Development)
get_filename_component(Python_Base_Dir ${Python_INCLUDE_DIRS} DIRECTORY)
#
set(AR_PYTHON_LIB_NUMBER python39)
set(AR_PYTHON_LIB_DIR ${Python_Base_Dir}) 
set(AR_PYTHON_INCLUDE_DIR ${Python_Base_Dir}/python3.9)

# setting up boost 
add_compile_definitions(BOOST_ALL_NO_LIB)
set(AR_BOOST_NAMESPACE boost) 
set(AR_BOOST_INCLUDE_DIR ${AR_AYONUSD_ROOT}/include)
set(BOOST_LIB_DIR ${AR_AYONUSD_ROOT}/lib)


# can be enabled if the USD binary's you`r using are build without the new CXX11 ABI
# set(GLIBCXX_USE_CXX11_ABI 1)
