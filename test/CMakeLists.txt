find_package(Catch2 CONFIG REQUIRED)
find_package(Python)

add_executable(httpmicroservice_test test.cpp)
target_link_libraries(
    httpmicroservice_test PRIVATE httpmicroservice Catch2::Catch2)

add_test(UnitTests httpmicroservice_test)

# Run client requests to test server
# python test/make_client_requests.py httpmicroservice_test
# add_test(
#     NAME test
#     COMMAND ${Python_EXECUTABLE}
#     ${CMAKE_CURRENT_SOURCE_DIR}/make_client_requests.py
#     $<TARGET_FILE:httpmicroservice_test>)