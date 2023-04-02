cmake_minimum_required(VERSION 3.14)

project(skyeTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(skye REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)

include(Catch)

# ---- Tests ----

add_executable(skye-test test.cpp test_service.cpp test_session.cpp)
target_link_libraries(
    skye-test PRIVATE
    skye::skye
    Catch2::Catch2WithMain
    fmt::fmt
)
target_compile_features(skye-test PRIVATE cxx_std_20)

catch_discover_tests(skye-test)

# ---- End-of-file commands ----

add_folders(Tests)
