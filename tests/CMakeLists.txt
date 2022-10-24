find_package(Catch2 REQUIRED)
# find_package(Python)

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

# Run client requests to test server
# python tests/make_client_requests.py usrv-test
# add_test(
#     NAME IntegrationTests
#     COMMAND ${Python_EXECUTABLE}
#     ${CMAKE_CURRENT_SOURCE_DIR}/make_client_requests.py
#     $<TARGET_FILE:usrv-test>)
