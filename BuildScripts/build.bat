dir ../

rmdir /s /q build
REM rmdir /s /q Resolvers

REM set HFS={Houdini install directory}
REM set COMPILEPLUGIN={Build Plugin Path / name starting at the BuildPlugins foulder dir}

cmake -S . -B build -DDEV=0 -DJTRACE=0 -DCMAKE_BUILD_TYPE=Release
cmake --build build --clean-first --config Release
cmake --install build   
