include(cmake/folders.cmake)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

option(ENABLE_EXAMPLES "Build example apps" ON)
if(ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()

option(ENABLE_BENCHMARKS "Build benchmarks" OFF)
if(ENABLE_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

option(ENABLE_COVERAGE "Enable coverage support separate from CTest's" OFF)
if(ENABLE_COVERAGE)
    include(cmake/coverage.cmake)
endif()

include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

add_folders(Project)
