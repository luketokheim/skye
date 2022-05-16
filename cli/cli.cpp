#include <httpappserver.hpp>

#include <iostream>

int main(int argc, char **argv)
{
    try {
        return httpappserver::run();
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    }

    return -1;
}