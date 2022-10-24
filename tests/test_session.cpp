#include <catch2/catch_test_macros.hpp>

#include <httpmicroservice/session.hpp>

#include "mock_sock.hpp"
#include "test.hpp"

#include <boost/asio/co_spawn.hpp>

#include <vector>

namespace asio = boost::asio;
namespace http = httpmicroservice::http;
namespace usrv = httpmicroservice;

TEST_CASE("async_read_some", "[mock_sock]")
{
    using buffer = std::vector<char>;

    asio::io_context ctx;
    test::mock_sock<buffer> s(ctx.get_executor());

    {
        const auto data = test::make_random_string<buffer>(1024);
        s.set_rx(data);

        const auto n = data.size();

        buffer buf(n);

        auto handler = [n](auto ec, auto bytes_transferred) {
            REQUIRE(!ec);
            REQUIRE(n == bytes_transferred);
        };

        s.async_read_some(asio::buffer(buf), handler);

        REQUIRE(data == buf);
    }
}

TEST_CASE("async_read", "[mock_sock]")
{
    using buffer = std::vector<char>;

    asio::io_context ctx;
    test::mock_sock<buffer> s(ctx.get_executor());

    {
        const auto data = test::make_random_string<buffer>(1024);
        s.set_rx(data);

        const auto n = data.size();

        buffer buf(n);

        auto handler = [n](auto ec, auto bytes_transferred) {
            REQUIRE(!ec);
            REQUIRE(n == bytes_transferred);
        };

        asio::async_read(s, asio::buffer(buf), handler);

        REQUIRE(data == buf);
    }
}

TEST_CASE("async_write_some", "[mock_sock]")
{
    using buffer = std::vector<char>;

    asio::io_context ctx;
    test::mock_sock<buffer> s(ctx.get_executor());

    {
        const auto data = test::make_random_string<buffer>(1024);

        const auto n = data.size();

        auto handler = [n](auto ec, auto bytes_transferred) {
            REQUIRE(!ec);
            REQUIRE(n == bytes_transferred);
        };

        s.async_write_some(asio::buffer(data), handler);

        REQUIRE(data == s.get_tx());
    }
}

TEST_CASE("async_write", "[mock_sock]")
{
    using buffer = std::vector<char>;

    asio::io_context ctx;
    test::mock_sock<buffer> s(ctx.get_executor());

    {
        const auto data = test::make_random_string<buffer>(1024);

        const auto n = data.size();

        auto handler = [n](auto ec, auto bytes_transferred) {
            REQUIRE(!ec);
            REQUIRE(n == bytes_transferred);
        };

        asio::async_write(s, asio::buffer(data), handler);

        REQUIRE(data == s.get_tx());
    }
}

TEST_CASE("session", "[session]")
{
    asio::io_context ctx;
    test::mock_sock<std::string> s(ctx.get_executor());

    const std::string data = "GET / HTTP/1.0\r\n\r\n";
    s.set_rx(data);

    const std::string body = test::make_random_string<std::string>(1024);

    int handler_called = 0;
    auto handler = [&body, &handler_called](
                       usrv::request req) -> asio::awaitable<usrv::response> {
        ++handler_called;

        usrv::response res(http::status::ok, req.version());
        res.body() = body;

        co_return res;
    };

    auto future = co_spawn(
        ctx,
        usrv::session(s, handler, std::make_optional<usrv::session_stats>()),
        asio::use_future);

    ctx.run();

    REQUIRE(future.valid());

    auto stats = future.get();
    REQUIRE(stats);
    REQUIRE(stats->num_request == 1);
    REQUIRE(stats->bytes_read == data.size());

    REQUIRE(s.get_tx().ends_with(body));
    REQUIRE(handler_called == 1);
}
