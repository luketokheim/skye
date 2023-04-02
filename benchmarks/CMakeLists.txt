cmake_minimum_required(VERSION 3.14)

project(skyeBenchmarks LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(skye REQUIRED)
endif()

find_package(benchmark REQUIRED)
find_package(fmt REQUIRED)

# ---- Benchmarks ----

add_executable(skye-bench bench.cpp bench_format.cpp bench_session.cpp)
target_compile_definitions(skye-bench PRIVATE BOOST_ALL_NO_LIB)
target_link_libraries(
    skye-bench PRIVATE
    skye::skye
    benchmark::benchmark
    fmt::fmt
)

# ---- End-of-file commands ----

add_folders(Benchmarks)
