# Variables that need to be set by the plugin ( the env variables will be checked by the Main CMakeLists.txt and an error will occur if one or more are missing )
# AR_PXR_INCLUDE_DIR
# AR_PXR_LIB_DIR
# AR_PYTHON_LIB_NUMBER
# AR_BOOST_INCLUDE_DIR
# BOOST_LIB_DIR
# AR_PYTHON_LIB_DIR
# AR_PYTHON_INCLUDE_DIR
# AR_PXR_LIB_PREFIX


# plugin dependent settings 
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
set(AR_PYTHON_LIB_NUMBER python39)
get_filename_component(Python_Base_Dir ${Python_INCLUDE_DIRS} DIRECTORY)
set(AR_PYTHON_LIB_DIR ${Python_Base_Dir}/libs)
set(AR_PYTHON_INCLUDE_DIR ${Python_Base_Dir}/include)

# setting up boost 
add_compile_definitions(BOOST_ALL_NO_LIB)
set(AR_BOOST_NAMESPACE boost) 
set(AR_BOOST_EXTEND -vc143-mt-x64-1_78) # boost has a name attachment under windows/Usd this allows you to overwrite the default name attachment 
set(AR_BOOST_INCLUDE_DIR ${AR_AYONUSD_ROOT}/include/boost-1_78)
set(BOOST_LIB_DIR ${AR_AYONUSD_ROOT}/lib)
