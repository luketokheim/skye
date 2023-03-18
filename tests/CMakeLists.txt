find_package(Catch2 3.1.0 REQUIRED CONFIG)

add_executable(skye-test test.cpp test_service.cpp test_session.cpp)

target_include_directories(skye-test PRIVATE .)

target_link_libraries(skye-test PRIVATE skye::skye Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(skye-test)
