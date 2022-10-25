#!/bin/sh

conan profile new gcov --detect
conan profile update settings.build_type=Debug gcov
conan install .. --build=missing --profile=gcov

cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

cmake --build .

ctest --output-on-failure

# gcovr bundled with jammy is buggy, use pip to get newer release
# pip install gcovr
python -m gcovr -r .. --filter ../include/ --filter ../src/  --html-details coverage.html
