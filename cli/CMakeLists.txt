add_executable(cli cli.cpp)
target_link_libraries(cli PRIVATE httpmicroservice)

install(TARGETS cli RUNTIME DESTINATION bin)
