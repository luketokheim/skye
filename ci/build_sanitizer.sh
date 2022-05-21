#!/bin/sh

conan profile new sanitizer --detect
conan profile update settings.build_type=Debug sanitizer
conan profile update settings.compiler=clang sanitizer
conan profile update settings.compiler.version=14 sanitizer
conan profile update settings.compiler.libcxx=libc++ sanitizer
conan install .. --build=missing --profile=sanitizer

cmake -B . -S .. -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++-14 \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,thread,signed-integer-overflow,null" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

cmake --build . --config Debug

ctest