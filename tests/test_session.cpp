#include <catch2/catch_test_macros.hpp>

#include <skye/session.hpp>

#include "mock_sock.hpp"
#include "test.hpp"

#include <boost/asio/co_spawn.hpp>

#include <vector>

namespace asio = boost::asio;
namespace http = boost::beast::http;

TEST_CASE("async_read_some", "[mock_sock]")
{
    using buffer = std::vector<char>;
    using tcp_socket = test::mock_sock<buffer, asio::io_context::executor_type>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

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
    using tcp_socket = test::mock_sock<buffer, asio::io_context::executor_type>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

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
    using tcp_socket = test::mock_sock<buffer, asio::io_context::executor_type>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

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
    using tcp_socket = test::mock_sock<buffer, asio::io_context::executor_type>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

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
    using buffer = std::string;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_socket = default_token::as_default_on_t<
        test::mock_sock<buffer, asio::io_context::executor_type>>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

    const buffer data = "GET / HTTP/1.0\r\n\r\n";
    s.set_rx(data);

    const buffer body = test::make_random_string<buffer>(1024);

    int handler_called = 0;
    auto handler = [&body, &handler_called](
                       skye::request req) -> asio::awaitable<skye::response> {
        ++handler_called;

        skye::response res(http::status::ok, req.version());
        res.body() = body;

        co_return res;
    };

    skye::session_metrics metrics;
    auto reporter = [&metrics](const skye::session_metrics& m) { metrics = m; };

    auto future = co_spawn(
        ctx.get_executor(), skye::session(s, handler, reporter),
        asio::use_future);

    REQUIRE(ctx.run() > 0);

    REQUIRE(future.valid());
    future.get();

    REQUIRE(metrics.num_request == 1);
    REQUIRE(metrics.bytes_read == data.size());

    REQUIRE(s.get_tx().ends_with(body));
    REQUIRE(handler_called == 1);
}

TEST_CASE("session_error", "[session]")
{
    using buffer = std::string;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_socket = default_token::as_default_on_t<
        test::mock_sock<buffer, asio::io_context::executor_type>>;

    asio::io_context ctx;
    tcp_socket s(ctx.get_executor());

    const buffer data = "GET / xxx HTTP/1.0\r\n\r\n";
    s.set_rx(data);

    bool handler_called = false;
    auto handler = [&handler_called](
                       skye::request req) -> asio::awaitable<skye::response> {
        handler_called = true;
        co_return skye::response{http::status::ok, req.version()};
    };

    skye::session_metrics metrics;
    auto reporter = [&metrics](const skye::session_metrics& m) { metrics = m; };

    auto future =
        co_spawn(ctx, skye::session(s, handler, reporter), asio::use_future);

    REQUIRE(ctx.run() > 0);

    REQUIRE(future.valid());
    future.get();

    REQUIRE(metrics.num_request == 0);
    REQUIRE(!handler_called);
}
