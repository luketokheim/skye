add_executable(usrv-hello hello.cpp)
target_link_libraries(
    usrv-hello PRIVATE httpmicroservice::httpmicroservice)

add_executable(usrv-producer producer.cpp)
target_link_libraries(
    usrv-producer PRIVATE httpmicroservice::httpmicroservice)

# add_executable(usrv-threaded threaded.cpp)
# target_link_libraries(
#     usrv-threaded PRIVATE httpmicroservice::httpmicroservice)

install(TARGETS usrv-hello RUNTIME)
