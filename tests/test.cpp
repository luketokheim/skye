#include <catch2/catch_test_macros.hpp>

#include <httpmicroservice.hpp>

#include <array>
#include <cstdlib>

namespace usrv = httpmicroservice;

TEST_CASE("make_response", "[usrv]")
{
    usrv::request req;
    auto res = usrv::make_response(req);

    REQUIRE(res.result_int() == 200);
    REQUIRE(req.version() == res.version());
}

TEST_CASE("getenv_port", "[usrv]")
{
    constexpr auto kName = "PORT";
    REQUIRE(unsetenv(kName) == 0);

    REQUIRE(usrv::getenv_port() == 8080);

    for (int port=1024; port<=65535; ++port) {
        const auto str = std::to_string(port);
        REQUIRE(setenv(kName, str.c_str(), 1) == 0);
    
        REQUIRE(usrv::getenv_port() == port);
    }

    constexpr std::array<int, 6> kBadRange = {-10, 0, 80, 1023, 65536, 100000};

    for (int port : kBadRange) {
        const auto str = std::to_string(port);
        REQUIRE(setenv(kName, str.c_str(), 1) == 0);

        REQUIRE_THROWS(usrv::getenv_port());
    }

    constexpr std::array<const char*, 4> kBadNumber = {"-dingo", "3.14 is pi", ".8080.1234", "nope"};

    for (auto str : kBadNumber) {
        REQUIRE(setenv(kName, str, 1) == 0);

        REQUIRE_THROWS(usrv::getenv_port());
    }
}
