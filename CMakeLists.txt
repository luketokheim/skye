# conan generator requires 3.15
cmake_minimum_required(VERSION 3.15)

project(httpmicroservice)

add_library(httpmicroservice src/service.cpp)
add_library(httpmicroservice::httpmicroservice ALIAS httpmicroservice)

target_compile_features(httpmicroservice PUBLIC cxx_std_20)
target_include_directories(httpmicroservice PUBLIC include)

# Set default construction of asio::io_context for single thread
target_compile_definitions(
    httpmicroservice
    PUBLIC
    BOOST_ASIO_NO_DEPRECATED
    BOOST_ASIO_CONCURRENCY_HINT_DEFAULT=1
    BOOST_ASIO_CONCURRENCY_HINT_1=BOOST_ASIO_CONCURRENCY_HINT_UNSAFE)

# g++-10 or newer on Linux
if(CMAKE_COMPILER_IS_GNUCXX)
    find_package(Threads)
    target_compile_options(httpmicroservice PUBLIC -fcoroutines)
    target_link_libraries(httpmicroservice PUBLIC ${CMAKE_THREAD_LIBS_INIT})
endif()

# clang++-13 or newer on Linux
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(httpmicroservice PUBLIC -stdlib=libc++)
    target_link_options(httpmicroservice PUBLIC -stdlib=libc++)
endif()

find_package(Boost 1.79 REQUIRED CONFIG)
find_package(fmt 9.0 REQUIRED CONFIG)

target_link_libraries(
    httpmicroservice
    PUBLIC
    Boost::boost
    fmt::fmt)

option(
    ENABLE_STANDALONE
    "Compile standalone app with static linked C/C++"
    OFF)
if(CMAKE_COMPILER_IS_GNUCXX AND ENABLE_STANDALONE)
    target_compile_options(httpmicroservice PUBLIC -static-libstdc++ -static)
    target_link_options(httpmicroservice PUBLIC -static-libstdc++ -static)
endif()

find_library(URING_LIBRARY uring)
option(
    ENABLE_IO_URING
    "Enable the io_uring backend on Linux if liburing is found"
    ON)
if(ENABLE_IO_URING AND URING_LIBRARY)
    target_compile_definitions(
        httpmicroservice
        PUBLIC
        BOOST_ASIO_HAS_IO_URING
        BOOST_ASIO_DISABLE_EPOLL)
    target_link_libraries(httpmicroservice PUBLIC uring)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    add_subdirectory(examples)

    install(DIRECTORY include DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/")
    install(TARGETS httpmicroservice)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
