add_executable(usrv-example example.cpp)
target_link_libraries(usrv-example PRIVATE httpmicroservice::httpmicroservice)

install(TARGETS usrv-example RUNTIME)
