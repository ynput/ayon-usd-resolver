@echo off
setlocal

REM dir ../

set "HOUDINI_BIN_DIR=C:\Program Files\Side Effects Software\Houdini 20.0.590\bin"
set "SCRIPT_DIR=C:\Users\lyonh\Downloads\HouWin\WindowsPy310Houdini20"

REM Set Usd/Python Variables for Resolver Load
set "USD_ASSET_RESOLVER=%SCRIPT_DIR%"

REM AYONUSDRESOLVER_RESOLVER_CONTEXT
REM PLUG_*
set "TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT"
set "LD_LIBRARY_PATH=%SCRIPT_DIR%\ayonUsdResolver\lib;%LD_LIBRARY_PATH%"
set "PXR_PLUGINPATH_NAME=%SCRIPT_DIR%\ayonUsdResolver\resources;%PXR_PLUGINPATH_NAME%"
set "PYTHONPATH=%SCRIPT_DIR%\ayonUsdResolver\lib\python;%PYTHONPATH%"



REM use those lines to set the right env varibles if your not using ayon launcher to launch houdini
set "AYON_SITE_ID=groovy-amiable-reindeer"
set "AYON_API_KEY=2d62dae97a8c13d73aec2ca7c513abf055a7a6a44b08066584b6c55a1f1ca97b"
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

"%HOUDINI_BIN_DIR%\hython.exe" HouWinTest.py

pause
endlocal
