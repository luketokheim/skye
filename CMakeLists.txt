cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    skye
    VERSION 0.10.0
    DESCRIPTION "Skye is an HTTP server framework for C++20"
    HOMEPAGE_URL "https://github.com/luketokheim/skye"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(skye_skye INTERFACE)
add_library(skye::skye ALIAS skye_skye)

set_property(
    TARGET skye_skye PROPERTY
    EXPORT_NAME skye
)

target_include_directories(
    skye_skye ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(skye_skye INTERFACE cxx_std_20)

# Target Windows 10 SDK from asio
if(WIN32)
    target_compile_definitions(skye_skye INTERFACE _WIN32_WINNT=0x0A00)
endif()

find_package(Boost REQUIRED)
target_link_libraries(skye_skye INTERFACE Boost::boost)

find_package(fmt QUIET)
if(fmt_FOUND)
    target_link_libraries(skye_skye INTERFACE fmt::fmt)
endif()

# Use io_uring on Linux. Requires liburing-dev package and kernel support.
find_library(URING_LIBRARY uring)
option(
    ENABLE_IO_URING
    "Enable the io_uring backend on Linux if liburing is found"
    OFF)
if(ENABLE_IO_URING AND URING_LIBRARY)
    target_compile_definitions(
        skye_skye
        INTERFACE
        BOOST_ASIO_HAS_IO_URING
        BOOST_ASIO_DISABLE_EPOLL)
    target_link_libraries(skye_skye INTERFACE uring)
endif()

# Enable AVX2 vectorization for Linux x64. Faster buffer copies!
option(ENABLE_ARCH "Build with Skylake CPU specific instructions" OFF)
if(ENABLE_ARCH)
    target_compile_options(skye_skye INTERFACE -march=skylake)
endif()

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT skye_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(
        AUTHOR_WARNING
        "Developer mode is intended for developers of skye"
    )
endif()

include(cmake/dev-mode.cmake)
