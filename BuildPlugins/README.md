# Build Plugins 

This folder holds build plugins used in the resolver compile process to determine software configurations.   
BuildPlugins are .cmake files the main Cmake script in the root directory pulls in at compile time.  

Every build plugin needs to define variables that will be tested at compile time. 
- Variables. For more info, look into one of the existing build scripts. They are annotated, so they serve as a reference for implementing more scripts in the future. 

  - AR_PXR_INCLUDE_DIR
  - AR_PXR_LIB_DIR
  - AR_PYTHON_LIB_NUMBER
  - AR_BOOST_INCLUDE_DIR
  - BOOST_LIB_DIR
  - AR_PYTHON_LIB_DIR
  - AR_PYTHON_INCLUDE_DIR
  - AR_PXR_LIB_PREFIX
- Your script can also request data as env variables or as similar as you wish. 
