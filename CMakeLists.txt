# conan generator requires 3.15
cmake_minimum_required(VERSION 3.15)

project(httpmicroservice)

add_library(httpmicroservice src/service.cpp)
add_library(httpmicroservice::httpmicroservice ALIAS httpmicroservice)

target_compile_features(httpmicroservice PUBLIC cxx_std_20)
target_include_directories(httpmicroservice PUBLIC include)

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

find_package(Boost REQUIRED)
find_package(fmt REQUIRED)

target_link_libraries(httpmicroservice PUBLIC Boost::boost fmt::fmt)

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
    "Enable the io_uring backend"
    OFF)
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

    set(LIBRARY_PUBLIC_HEADERS
        include/httpmicroservice.hpp
        include/httpmicroservice/format.hpp
        include/httpmicroservice/service.hpp
        include/httpmicroservice/session.hpp
        include/httpmicroservice/types.hpp)

    set_target_properties(
        httpmicroservice PROPERTIES PUBLIC_HEADER "${LIBRARY_PUBLIC_HEADERS}")

    install(TARGETS httpmicroservice ARCHIVE PUBLIC_HEADER)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
