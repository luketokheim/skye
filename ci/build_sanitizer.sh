conan profile update settings.build_type=Debug default
conan profile update settings.compiler=clang default
conan profile update settings.compiler.version=14 default
conan profile update settings.compiler.libcxx=libc++ default
conan install .. --build=missing

cmake -B . -S .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_CXX_EXTENSIONS=OFF -DENABLE_FUZZER=OFF

 -DCMAKE_CXX_FLAGS="-fsanitize=fuzzer-no-link,address,signed-integer-overflow,null" 

cmake --build . --config Debug

ctest