# Minimum version for target_sources(... FILE_SET HEADERS ...)
cmake_minimum_required(VERSION 3.23)

project(skye)

add_library(skye src/service.cpp)
add_library(skye::skye ALIAS skye)

target_compile_features(skye PUBLIC cxx_std_20)
target_include_directories(skye PUBLIC include)

# g++-10 or newer on Linux
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_package(Threads)
    target_compile_options(skye PUBLIC -fcoroutines)
    target_link_libraries(skye PUBLIC ${CMAKE_THREAD_LIBS_INIT})
endif()

# clang++-13 or newer on Linux
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(skye PUBLIC -stdlib=libc++)
    target_link_options(skye PUBLIC -stdlib=libc++)
endif()

if(WIN32)
    target_compile_definitions(skye PUBLIC _WIN32_WINNT=0x0A00)
endif()

find_package(Boost 1.79 REQUIRED)
target_link_libraries(skye PUBLIC Boost::boost)

find_package(fmt 9.0 QUIET)
if(fmt_FOUND)
    target_link_libraries(skye PUBLIC fmt::fmt)
endif()

option(
    ENABLE_STANDALONE
    "Compile standalone app with static linked libstdc++"
    OFF)
if(CMAKE_COMPILER_IS_GNUCXX AND ENABLE_STANDALONE)
    target_compile_definitions(skye PUBLIC FMT_HEADER_ONLY=1)
    target_compile_options(skye PUBLIC -static-libstdc++ -static)
    target_link_options(skye PUBLIC -static-libstdc++ -static)
endif()

target_sources(
    skye PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES
    include/skye.hpp
    include/skye/format.hpp
    include/skye/service.hpp
    include/skye/session.hpp
    include/skye/types.hpp)

find_library(URING_LIBRARY uring)
option(
    ENABLE_IO_URING
    "Enable the io_uring backend on Linux if liburing is found"
    OFF)
if(ENABLE_IO_URING AND URING_LIBRARY)
    target_compile_definitions(
        skye
        PUBLIC
        BOOST_ASIO_HAS_IO_URING
        BOOST_ASIO_DISABLE_EPOLL)
    target_link_libraries(skye PUBLIC uring)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    add_subdirectory(examples)

    install(TARGETS skye FILE_SET HEADERS)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
