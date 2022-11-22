find_package(Catch2 3.1.0 REQUIRED CONFIG)

add_executable(
    usrv-test
    test.cpp
    test_service.cpp
    test_session.cpp)

target_include_directories(usrv-test PRIVATE .)

target_link_libraries(
    usrv-test PRIVATE
    httpmicroservice::httpmicroservice
    Catch2::Catch2WithMain)

add_test(UnitTests usrv-test)
