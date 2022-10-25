#!/bin/sh

export CC=clang
export CXX=clang++

conan profile new sanitizer --detect
conan profile update settings.build_type=Debug sanitizer
conan profile update settings.compiler.libcxx=libc++ sanitizer
conan install .. --build=missing --profile=sanitizer

cmake .. -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address" \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# thread,signed-integer-overflow,null

cmake --build .

ctest --output-on-failure
