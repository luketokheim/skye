#!/bin/sh

export CC=clang-14
export CXX=clang++-14

conan profile new sanitizer --detect
conan profile update settings.build_type=Debug sanitizer
conan profile update settings.compiler=clang sanitizer
conan profile update settings.compiler.version=14 sanitizer
conan profile update settings.compiler.libcxx=libc++ sanitizer
conan install .. --build=missing --profile=sanitizer

cmake .. -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++-14 \
  -DCMAKE_CXX_FLAGS="-fsanitize=address" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# thread,signed-integer-overflow,null

cmake --build . --config Debug -j 8

ctest -C Debug
