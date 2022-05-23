#!/bin/sh

conan profile new tidy --detect
conan profile update settings.build_type=Debug tidy
conan profile update settings.compiler=clang tidy
conan profile update settings.compiler.version=14 tidy
conan profile update settings.compiler.libcxx=libc++ tidy
conan install .. --build=missing --profile=tidy

cmake -B . -S .. -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++-14 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

cmake --build . --config Debug

ctest -C Debug
