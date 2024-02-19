@echo off
setlocal

REM Get the directory of the script
for %%I in ("%~dp0.") do set "SCRIPT_DIR=%%~fI"
set "SCRIPT_DIR=%SCRIPT_DIR%\dist"

REM Set Usd/Python Variables for Resolver Load
set "USD_ASSET_RESOLVER=%SCRIPT_DIR%"
set "TF_DEBUG=AYONUSDRESOLVER_RESOLVER"
set "LD_LIBRARY_PATH=%SCRIPT_DIR%\ayonUsdResolver\lib;%LD_LIBRARY_PATH%"
set "PXR_PLUGINPATH_NAME=%SCRIPT_DIR%\ayonUsdResolver\resources;%PXR_PLUGINPATH_NAME%"
set "PYTHONPATH=%SCRIPT_DIR%\ayonUsdResolver\lib\python;%PYTHONPATH%"


set "AYON_SITE_ID=groovy-amiable-reindeer"
set "AYON_API_KEY=f2edece31ee8376292c4d545d711ca36cf6a7f47485f565bf356512b7963bec1"
set "AYON_SERVER_URL=http://192.168.178.42:5000"

REM Print environment variables
echo USD_ASSET_RESOLVER=%USD_ASSET_RESOLVER%
echo TF_DEBUG=%TF_DEBUG%
echo LD_LIBRARY_PATH=%LD_LIBRARY_PATH%
echo PXR_PLUGINPATH_NAME=%PXR_PLUGINPATH_NAME%
echo PYTHONPATH=%PYTHONPATH%
echo AYON_API_KEY=%AYON_API_KEY%
echo AYON_SITE_ID=%AYON_SITE_ID%
echo AYON_SERVER_URL=%AYON_SERVER_URL%

REM Command to execute
"C:\Program Files\Side Effects Software\Houdini 20.0.590\bin\hython.exe" test.py


endlocal
