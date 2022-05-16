# cxx_std_20 requires 3.12
# conan generator requires 3.15
cmake_minimum_required(VERSION 3.15)

project(httpappserver CXX)

add_library(
    httpappserver
    src/server.cpp)

target_compile_features(httpappserver PUBLIC cxx_std_20)
target_include_directories(httpappserver PUBLIC include)

# g++10 or newer on Linux
if(CMAKE_COMPILER_IS_GNUCXX)
    find_package(Threads)
    target_compile_options(httpappserver PUBLIC -fcoroutines)  
    target_link_libraries(httpappserver PUBLIC ${CMAKE_THREAD_LIBS_INIT})
endif()

find_package(Boost REQUIRED)
target_link_libraries(httpappserver PUBLIC Boost::boost)

add_subdirectory(cli)
