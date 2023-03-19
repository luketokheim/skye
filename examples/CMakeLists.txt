find_package(SQLite3 CONFIG)
if(SQLite3_FOUND)
    add_executable(skye-database database.cpp)
    target_link_libraries(skye-database PRIVATE skye::skye SQLite::SQLite3)

    install(TARGETS skye-database RUNTIME)
endif()

add_executable(skye-echo echo.cpp)
target_link_libraries(skye-echo PRIVATE skye::skye)

install(TARGETS skye-echo RUNTIME)

add_executable(skye-hello hello.cpp)
target_link_libraries(skye-hello PRIVATE skye::skye)

install(TARGETS skye-hello RUNTIME)

add_executable(skye-producer producer.cpp)
target_link_libraries(skye-producer PRIVATE skye::skye)

install(TARGETS skye-producer RUNTIME)
