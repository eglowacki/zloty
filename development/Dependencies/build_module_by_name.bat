echo off
echo mkdir builds\%1
mkdir builds\%1
cd builds\%1
echo ----------------------------------------------------------
echo ---------------- Starting cmake in builds\%1...
cmake ../../%1 -G"Visual Studio 18 2026" -Ax64 -Wno-dev

echo ----------------------------------------------------------
echo ---------------- Building '%1' in Debug mode...
cmake --build . --config Debug

echo ----------------------------------------------------------
echo ---------------- Building '%1' in Release mode...
cmake --build . --config Release

echo ----------------------------------------------------------
echo ---------------- Finished building '%1'.
cd ..\..
