set HFS=C:\Program Files\Side Effects Software\Houdini 20.0.590





cmake -S . -B build -DDEV=0 -DJTRACE=0 -DCMAKE_BUILD_TYPE=Release

 
cmake --build build --clean-first --parallel --config Release

cmake --install build   