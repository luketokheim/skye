#include <httpmicroservice.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdlib>

namespace usrv = httpmicroservice;

#if !defined(_WIN32)

TEST_CASE("getenv_port", "[usrv]")
{
    // Do not run this one on Windows because environment vars work completely
    // differently. SetEnvironmentVariable does not update the vars that we read
    // back with std::getenv. This is intended for Docker Linux containers so
    // leave Windows out.
    constexpr auto kName = "PORT";

    REQUIRE(unsetenv(kName) == 0);

    REQUIRE(usrv::getenv_port() == 8080);

    for (int port = 1024; port <= 65535; ++port) {
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

    constexpr std::array<const char*, 4> kBadNumber = {
        "-dingo", "3.14 is pi", ".8080.1234", "nope"};

    for (auto str : kBadNumber) {
        REQUIRE(setenv(kName, str, 1) == 0);

        REQUIRE_THROWS(usrv::getenv_port());
    }
}

#endif // _WIN32

TEST_CASE("format", "[usrv]")
{
    usrv::session_stats stats{};

    const std::string str = fmt::format("{}", stats);

    REQUIRE(!str.empty());
}