add_executable(skye-echo echo.cpp)
target_link_libraries(skye-echo PRIVATE skye::skye)

install(TARGETS skye-echo RUNTIME)

add_executable(skye-hello hello.cpp)
target_link_libraries(skye-hello PRIVATE skye::skye)

install(TARGETS skye-hello RUNTIME)

add_executable(skye-producer producer.cpp)
target_link_libraries(skye-producer PRIVATE skye::skye)

install(TARGETS skye-producer RUNTIME)
