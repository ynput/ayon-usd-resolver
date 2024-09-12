REM in case you use Pyenv // Pyenv exec works

cd ../

rmdir /s /q build
rmdir /s /q Resolvers/AyonUsdWin/AyonUsd23_5_py39

set AyonUsdRoot=
set COMPILEPLUGIN=

cmake -S . -B build -DDEV=0 -DJTRACE=0 -DCMAKE_BUILD_TYPE=Release
cmake --build build --clean-first --config Release
cmake --install build
