#!/bin/sh

conan profile new coverage --detect
conan profile update settings.build_type=Debug coverage
conan profile update settings.compiler=clang coverage
conan profile update settings.compiler.version=14 coverage
conan profile update settings.compiler.libcxx=libc++ coverage
conan install .. --build=missing --profile=coverage

cmake -B . -S .. -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++-14 \
  -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping" \
  -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate -fcoverage-mapping" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

cmake --build . --config Debug

export LLVM_PROFILE_FILE=coverage.profraw
ctest -C Debug
llvm-profdata-14 merge -sparse test/coverage.profraw -o test/coverage.profdata
llvm-cov-14 show ./test/httpmicroservice_test -instr-profile=test/coverage.profdata >coverage.txt
