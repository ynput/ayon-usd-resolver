@echo off
setlocal

REM set "HOUDINI_BIN_DIR=C:\Program Files\Side Effects Software\Houdini 20.0.590\bin"

REM Get the directory of the script
REM set "SCRIPT_DIR=%CD%"
REM set "SCRIPT_DIR=%SCRIPT_DIR%\Resolvers\{BuildPlugin}"

REM Set Usd/Python Variables for Resolver Load
set "USD_ASSET_RESOLVER=%SCRIPT_DIR%"
set "TF_DEBUG=AYONUSDRESOLVER_RESOLVER"
set "LD_LIBRARY_PATH=%SCRIPT_DIR%\ayonUsdResolver\lib;%LD_LIBRARY_PATH%"
set "PXR_PLUGINPATH_NAME=%SCRIPT_DIR%\ayonUsdResolver\resources;%PXR_PLUGINPATH_NAME%"
set "PYTHONPATH=%SCRIPT_DIR%\ayonUsdResolver\lib\python;%PYTHONPATH%"


REM use those lines to set the right env varibles if your not using ayon launcher to launch houdini
REM set "AYON_SITE_ID={LocalSiteId}"
REM set "AYON_API_KEY={AcountApiKey}"
REM set "AYON_SERVER_URL={serverAdress}"


REM Print environment variables
echo USD_ASSET_RESOLVER=%USD_ASSET_RESOLVER%
echo TF_DEBUG=%TF_DEBUG%
echo LD_LIBRARY_PATH=%LD_LIBRARY_PATH%
echo PXR_PLUGINPATH_NAME=%PXR_PLUGINPATH_NAME%
echo PYTHONPATH=%PYTHONPATH%
echo AYON_API_KEY=%AYON_API_KEY%
echo AYON_SITE_ID=%AYON_SITE_ID%
echo AYON_SERVER_URL=%AYON_SERVER_URL%

"%HOUDINI_BIN_DIR%\houdini.exe"

endlocal
