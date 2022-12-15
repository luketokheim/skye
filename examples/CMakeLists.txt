add_executable(usrv-echo echo.cpp)
target_link_libraries(
    usrv-echo PRIVATE httpmicroservice::httpmicroservice)

install(TARGETS usrv-echo RUNTIME)

add_executable(usrv-hello hello.cpp)
target_link_libraries(
    usrv-hello PRIVATE httpmicroservice::httpmicroservice)

install(TARGETS usrv-hello RUNTIME)

add_executable(usrv-producer producer.cpp)
target_link_libraries(
    usrv-producer PRIVATE httpmicroservice::httpmicroservice)

install(TARGETS usrv-producer RUNTIME)
