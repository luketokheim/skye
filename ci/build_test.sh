#!/bin/sh

conan profile new default --detect
conan install .. --build=missing

cmake -B . -S .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

cmake --build . --config Release

ctest -C Release
