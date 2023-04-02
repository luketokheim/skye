cmake_minimum_required(VERSION 3.14)

project(skyeExamples LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(skye REQUIRED)
endif()

find_package(fmt QUIET)
find_package(SQLite3 QUIET)

# ---- Examples ----

add_executable(skye-hello hello.cpp)
target_link_libraries(
    skye-hello PRIVATE
    skye::skye
)

# Echo and Producer examples require fmt
if(fmt_FOUND)
    add_executable(skye-echo echo.cpp)
    target_link_libraries(
        skye-echo PRIVATE
        skye::skye
        fmt::fmt
    )

    add_executable(skye-producer producer.cpp)
    target_link_libraries(
        skye-producer PRIVATE
        skye::skye
        fmt::fmt
    )
endif()

# Database example requires fmt and sqlite3
if(fmt_FOUND AND SQLite3_FOUND)
    add_executable(skye-database database.cpp)
    target_link_libraries(
        skye-database PRIVATE
        skye::skye
        fmt::fmt
        SQLite::SQLite3
    )
endif()

# ---- End-of-file commands ----

add_folders(Examples)
