@ echo off

echo [36m ----- Building Plugin -----
echo [37m

mkdir build && cd build
cmake ..
cmake -B ./build -S .. --build --target ALL_BUILD --config Release
cmake --install .

echo [36m ----- End of script -----
echo [37m
pause