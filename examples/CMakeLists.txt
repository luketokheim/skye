add_executable(usrv-example example.cpp)
target_link_libraries(usrv-example PRIVATE httpmicroservice::httpmicroservice)

add_executable(usrv-basic basic.cpp)
target_link_libraries(usrv-basic PRIVATE Boost::boost fmt::fmt)

install(TARGETS usrv-example RUNTIME)
