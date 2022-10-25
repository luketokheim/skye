#!/bin/sh

export CC=clang
export CXX=clang++

conan profile new tidy --detect
conan profile update settings.build_type=Release tidy
conan profile update settings.compiler.libcxx=libc++ tidy
conan install .. --build=missing --profile=tidy

cmake .. -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_CLANG_TIDY=clang-tidy \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
  -DBUILD_TESTING=OFF

cmake --build .
