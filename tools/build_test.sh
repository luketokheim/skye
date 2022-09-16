#!/bin/sh

conan profile new default --detect
conan install .. --build=missing

cmake .. \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

cmake --build . --config Release -j 8

ctest -C Release
