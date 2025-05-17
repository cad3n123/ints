@echo off
REM Usage: build.bat path\to\vcpkg\scripts\buildsystems\vcpkg.cmake

if "%1"=="" (
    echo Please provide the path to vcpkg toolchain file.
    echo Example: build.bat C:\vcpkg\scripts\buildsystems\vcpkg.cmake
    exit /b 1
)

set TOOLCHAIN_FILE=%1

if not exist build (
    mkdir build
)
cd build

cmake .. -DCMAKE_TOOLCHAIN_FILE=%TOOLCHAIN_FILE%
cmake --build .
cd ..
