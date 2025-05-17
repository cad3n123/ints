#!/bin/bash
# Usage: ./build.sh /path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

if [ -z "$1" ]; then
    echo "Please provide the path to the vcpkg toolchain file."
    echo "Example: ./build.sh ~/vcpkg/scripts/buildsystems/vcpkg.cmake"
    exit 1
fi

TOOLCHAIN_FILE=$1

mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
cmake --build .
cd ..
