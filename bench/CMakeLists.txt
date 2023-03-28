find_package(benchmark REQUIRED)

add_executable(skye-bench bench.cpp bench_format.cpp bench_session.cpp)

target_compile_definitions(skye-bench PRIVATE BOOST_ALL_NO_LIB)

target_link_libraries(skye-bench PRIVATE skye::skye benchmark::benchmark)

add_test(Benchmarks skye-bench)
