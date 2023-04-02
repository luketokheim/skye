include(CMakeFindDependencyMacro)
find_dependency(Boost)
find_dependency(fmt)

include("${CMAKE_CURRENT_LIST_DIR}/skyeTargets.cmake")
