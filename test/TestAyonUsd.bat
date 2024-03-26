@echo off
setlocal

REM dir ../

set HOUDINI_BIN_DIR=C:/Program Files/Side Effects Software/Houdini 20.0.590/bin
set COMPILEPLUGIN=AyonUsdWin/AyonUsd23_5_py39

set "SCRIPT_DIR=%CD%"
set "SCRIPT_DIR=%SCRIPT_DIR%\Resolvers\AyonUsdWin\AyonUsd23_5_py39"

set "PYTHONPATH=C:\Users\lyonh\Desktop\ynput\ayon-usd\downloads\usd-23.05_win64_py39\lib\python;%PYTHONPATH%"
set "PATH=C:\Users\lyonh\Desktop\ynput\ayon-usd\downloads\usd-23.05_win64_py39\bin;C:\Users\lyonh\Desktop\ynput\ayon-usd\downloads\usd-23.05_win64_py39\lib;%PATH%" 

REM Set Usd/Python Variables for Resolver Load
set "USD_ASSET_RESOLVER=%SCRIPT_DIR%"
set "TF_DEBUG=AYONUSDRESOLVER_RESOLVER_CONTEXT"
set "LD_LIBRARY_PATH=%SCRIPT_DIR%\ayonUsdResolver\lib;%LD_LIBRARY_PATH%"
set "PXR_PLUGINPATH_NAME=%SCRIPT_DIR%\ayonUsdResolver\resources;%PXR_PLUGINPATH_NAME%"
set "PYTHONPATH=%SCRIPT_DIR%\ayonUsdResolver\lib\python;%PYTHONPATH%"
set "PATH=%SCRIPT_DIR%\ayonUsdResolver\lib\python;%PATH%"


REM use these lines to set the right env variable's if your not using AYON launcher to launch Houdini
set "AYON_SITE_ID=groovy-amiable-reindeer"
set "AYON_API_KEY=2d62dae97a8c13d73aec2ca7c513abf055a7a6a44b08066584b6c55a1f1ca97b"
set "AYON_SERVER_URL=http://192.168.178.42:5000"


REM Print environment variables
echo PATH=%PATH%
echo USD_ASSET_RESOLVER=%USD_ASSET_RESOLVER%
echo TF_DEBUG=%TF_DEBUG%
echo LD_LIBRARY_PATH=%LD_LIBRARY_PATH%
echo PXR_PLUGINPATH_NAME=%PXR_PLUGINPATH_NAME%
echo PYTHONPATH=%PYTHONPATH%
echo AYON_API_KEY=%AYON_API_KEY%
echo AYON_SITE_ID=%AYON_SITE_ID%
echo AYON_SERVER_URL=%AYON_SERVER_URL%
 
"python" test/AyonUsdTest.py
endlocal
