if(NOT DEFINED AR_PXR_INCLUDE_DIR)
    message(FATAL_ERROR "AR_PXR_INCLUDE_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PXR_LIB_DIR)
    message(FATAL_ERROR "AR_PXR_LIB_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PXR_LIB_PREFIX)
    message(FATAL_ERROR "AR_PXR_LIB_PREFIX is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PYTHON_LIB_NUMBER)
    message(FATAL_ERROR "AR_PYTHON_LIB_NUMBER is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PYTHON_LIB_DIR)
    message(FATAL_ERROR "AR_PYTHON_LIB_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_PYTHON_INCLUDE_DIR)
    message(FATAL_ERROR "AR_PYTHON_INCLUDE_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED AR_BOOST_INCLUDE_DIR)
    message(FATAL_ERROR "AR_BOOST_INCLUDE_DIR is not defined. Please set it before continuing.")
endif()
if(NOT DEFINED BOOST_LIB_DIR)
    message(FATAL_ERROR "BOOST_LIB_DIR is not defined. Please set it before continuing.")
endif()

message(STATUS "AR_PXR_INCLUDE_DIR path: ${AR_PXR_INCLUDE_DIR}")
message(STATUS "AR_PXR_LIB_DIR path: ${AR_PXR_LIB_DIR}")
message(STATUS "AR_PXR_LIB_PREFIX path: ${AR_PXR_LIB_PREFIX}")
message(STATUS "AR_PYTHON_LIB_NUMBER path: ${AR_PYTHON_LIB_NUMBER}")
message(STATUS "AR_PYTHON_LIB_DIR path: ${AR_PYTHON_LIB_DIR}")
message(STATUS "AR_PYTHON_INCLUDE_DIR path: ${AR_PYTHON_INCLUDE_DIR}")
message(STATUS "AR_BOOST_INCLUDE_DIR path: ${AR_BOOST_INCLUDE_DIR}")
message(STATUS "BOOST_LIB_DIR path: ${BOOST_LIB_DIR}")