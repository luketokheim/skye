add_executable(cli cli.cpp)
target_link_libraries(cli PRIVATE httpappserver)

install(TARGETS cli RUNTIME DESTINATION bin)
