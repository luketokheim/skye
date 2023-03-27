# 3.23 for better install target_sources(... FILE_SET HEADERS ...)
cmake_minimum_required(VERSION 3.15)

project(skye)

add_library(skye INTERFACE)
add_library(skye::skye ALIAS skye)

target_compile_features(skye INTERFACE cxx_std_20)
target_include_directories(skye INTERFACE include)

# g++-10 or newer on Linux
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_package(Threads)
    target_compile_options(skye INTERFACE -fcoroutines)
    target_link_libraries(skye INTERFACE ${CMAKE_THREAD_LIBS_INIT})
endif()

# clang++-13 or newer on Linux
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(skye INTERFACE -stdlib=libc++)
    target_link_options(skye INTERFACE -stdlib=libc++)
endif()

if(WIN32)
    target_compile_definitions(skye INTERFACE _WIN32_WINNT=0x0A00)
endif()

find_package(Boost 1.79 REQUIRED)
target_link_libraries(skye INTERFACE Boost::boost)

find_package(fmt 9.0 QUIET)
if(fmt_FOUND)
    target_link_libraries(skye INTERFACE fmt::fmt)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
    target_sources(
        skye INTERFACE FILE_SET HEADERS
        BASE_DIRS include
        FILES
        include/skye.hpp
        include/skye/format.hpp
        include/skye/service.hpp
        include/skye/session.hpp
        include/skye/types.hpp
        include/skye/utility.hpp)
endif()

find_library(URING_LIBRARY uring)
option(
    ENABLE_IO_URING
    "Enable the io_uring backend on Linux if liburing is found"
    OFF)
if(ENABLE_IO_URING AND URING_LIBRARY)
    target_compile_definitions(
        skye
        INTERFACE
        BOOST_ASIO_HAS_IO_URING
        BOOST_ASIO_DISABLE_EPOLL)
    target_link_libraries(skye INTERFACE uring)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
        install(TARGETS skye FILE_SET HEADERS)
    else()
        install(TARGETS skye)
        install(DIRECTORY include DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/")
    endif()
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(examples)
    add_subdirectory(tests)
endif()
