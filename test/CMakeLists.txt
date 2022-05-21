find_package(Catch2 CONFIG REQUIRED)
find_package(Python)

add_executable(httpmicroservice_test test.cpp test_session.cpp)
target_include_directories(httpmicroservice_test PRIVATE .)
target_link_libraries(
    httpmicroservice_test PRIVATE httpmicroservice Catch2::Catch2)

add_test(UnitTests httpmicroservice_test)

option(
    ENABLE_TEST_FUZZER
    "Enable fuzzer test"
    OFF)
if(ENABLE_TEST_FUZZER)
    add_executable(httpmicroservice_fuzzer test_fuzzer.cpp)
    target_compile_options(
        httpmicroservice_fuzzer PRIVATE -fsanitize=fuzzer)
    target_link_options(httpmicroservice_fuzzer PRIVATE -fsanitize=fuzzer)
    target_link_libraries(httpmicroservice_fuzzer PRIVATE httpmicroservice)
    
    add_test(FuzzerTests httpmicroservice_fuzzer)
endif()

# Run client requests to test server
# python test/make_client_requests.py httpmicroservice_test
# add_test(
#     NAME test
#     COMMAND ${Python_EXECUTABLE}
#     ${CMAKE_CURRENT_SOURCE_DIR}/make_client_requests.py
#     $<TARGET_FILE:httpmicroservice_test>)