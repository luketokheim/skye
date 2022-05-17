# cxx_std_20 requires 3.12
# conan generator requires 3.15
cmake_minimum_required(VERSION 3.15)

project(httpappserver CXX)

add_library(
    httpappserver
    src/httpappserver.cpp)

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

option(
    ENABLE_STANDALONE
    "Compile standalone app with static linked C/C++"
    OFF)
if(CMAKE_COMPILER_IS_GNUCXX AND ENABLE_STANDALONE)
    target_compile_options(httpappserver PUBLIC -static-libstdc++ -static)
    target_link_options(httpappserver PUBLIC -static-libstdc++ -static)
endif()

option(
    ENABLE_IO_URING
    "Enable the io_uring backend"
    OFF)
if(CMAKE_COMPILER_IS_GNUCXX AND ENABLE_IO_URING)
    target_compile_definitions(
        httpappserver PUBLIC BOOST_ASIO_HAS_IO_URING BOOST_ASIO_DISABLE_EPOLL)
    target_link_libraries(httpappserver PUBLIC uring)
endif()

add_subdirectory(cli)
