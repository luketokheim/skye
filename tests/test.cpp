#include <skye/format.hpp>
#include <skye/utility.hpp>

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>

#include <array>
#include <cstdlib>
#include <string>

#if !defined(_WIN32)

TEST_CASE("getenv_port", "[skye][utility]")
{
    // Do not run this one on Windows because environment vars work completely
    // differently. SetEnvironmentVariable does not update the vars that we read
    // back with std::getenv. This is intended for Docker Linux containers so
    // leave Windows out.
    constexpr auto kName = "PORT";
    constexpr int kPort = 8080;

    REQUIRE(unsetenv(kName) == 0);

    REQUIRE(skye::getenv_port() == kPort);

    for (int port = 1024; port <= 65535; ++port) {
        const auto str = std::to_string(port);
        REQUIRE(setenv(kName, str.c_str(), 1) == 0);

        REQUIRE(skye::getenv_port() == port);
    }

    constexpr std::array<int, 7> kBadRange = {-10,  0,     80,    443,
                                              1023, 65536, 100000};

    for (const int port : kBadRange) {
        const auto str = std::to_string(port);
        REQUIRE(setenv(kName, str.c_str(), 1) == 0);

        REQUIRE(skye::getenv_port() == kPort);
    }

    constexpr std::array<const char*, 4> kBadNumber = {
        "-dingo", "3.14 is pi", ".8181.1234", "nope"};

    for (const char* str : kBadNumber) {
        REQUIRE(setenv(kName, str, 1) == 0);

        REQUIRE(skye::getenv_port() == kPort);
    }
}

#endif // _WIN32

TEST_CASE("SessionMetrics", "[skye][format]")
{
    skye::SessionMetrics metrics;

    const std::string str = fmt::format("{}", metrics);

    REQUIRE(!str.empty());
    REQUIRE(str.starts_with("{"));
    REQUIRE(str.ends_with("}"));
}