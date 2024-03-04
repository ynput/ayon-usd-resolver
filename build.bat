rmdir /s /q build
rmdir /s /q Resolvers

set HFS=C:\Program Files\Side Effects Software\Houdini 20.0.590
set COMPILEPLUGIN=HouWin/WindowsPy310Houdini20

cmake -S . -B build -DDEV=0 -DJTRACE=0 -DCMAKE_BUILD_TYPE=Release
 
cmake --build build --clean-first --config Release

cmake --install build   