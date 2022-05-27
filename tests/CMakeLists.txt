find_package(Catch2 CONFIG REQUIRED)
find_package(Python)

add_executable(
    usrv-test test.cpp test_integration.cpp test_session.cpp)
target_include_directories(usrv-test PRIVATE .)
target_link_libraries(
    usrv-test PRIVATE
    httpmicroservice::httpmicroservice
    Catch2::Catch2)

add_test(UnitTests usrv-test)

option(
    ENABLE_TEST_FUZZER
    "Enable fuzzer test"
    OFF)
if(ENABLE_TEST_FUZZER)
    add_executable(usrv-fuzzer test_fuzzer.cpp)
    target_compile_options(
        usrv-fuzzer PRIVATE -fsanitize=fuzzer)
    target_link_options(usrv-fuzzer PRIVATE -fsanitize=fuzzer)
    target_link_libraries(
        usrv-fuzzer PRIVATE httpmicroservice::httpmicroservice)
    
    add_test(FuzzerTests usrv-fuzzer)
endif()

# Run client requests to test server
# python test/make_client_requests.py httpmicroservice_test
# add_test(
#     NAME test
#     COMMAND ${Python_EXECUTABLE}
#     ${CMAKE_CURRENT_SOURCE_DIR}/make_client_requests.py
#     $<TARGET_FILE:httpmicroservice_test>)