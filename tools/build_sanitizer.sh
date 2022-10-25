#!/bin/sh

export CC=clang-14
export CXX=clang++-14

export ASAN_OPTIONS=detect_leaks=1:color=always
export TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1

conan profile new sanitizer --detect
conan profile update settings.build_type=Debug sanitizer
conan profile update settings.compiler.libcxx=libc++ sanitizer
conan install .. --build=missing --profile=sanitizer

for name in "address" "thread"
do
  cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=${name}" \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

  cmake --build . --clean-first

  ctest --output-on-failure
done
