# Required for the conan toolchain generator.
cmake_minimum_required(VERSION 3.15)

project(skye)

include(GNUInstallDirs)

add_library(skye INTERFACE)
add_library(skye::skye ALIAS skye)

target_compile_features(skye INTERFACE cxx_std_20)
target_include_directories(
    skye
    INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

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

find_package(Boost REQUIRED)
target_link_libraries(skye INTERFACE Boost::boost)

find_package(fmt QUIET)
if(fmt_FOUND)
    target_link_libraries(skye INTERFACE fmt::fmt)
endif()

option(ENABLE_ARCH "Build with CPU specific instructions" OFF)
if(ENABLE_ARCH)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(skye INTERFACE -march=skylake)
    endif()
endif()

option(ENABLE_BENCHMARKS "Build and run benchmarks with testing" OFF)

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

    # Install header files.
    install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    
    # Install example apps and export target info.
    install(
        TARGETS skye
        EXPORT skye-targets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # Install the target file.
    install(
        EXPORT skye-targets
        NAMESPACE skye::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(examples)
    add_subdirectory(tests)
    if(ENABLE_BENCHMARKS)
        add_subdirectory(bench)
    endif()
endif()
