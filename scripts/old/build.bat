cd ../

rmdir /s /q build

cmake -S . -B build -DDEV=0 -DJTRACE=0 -DCMAKE_BUILD_TYPE=Release
cmake --build build --clean-first --config Release
cmake --install build
