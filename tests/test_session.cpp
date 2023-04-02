#include <skye/session.hpp>

#include "mock_sock.hpp"
#include "test.hpp"

#include <boost/asio.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

namespace asio = boost::asio;
namespace http = boost::beast::http;

TEST_CASE("async_read_some", "[test][MockSock]")
{
    using buffer = std::vector<char>;
    using tcp_socket = test::MockSock<buffer, asio::io_context::executor_type>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

    {
        const auto data = test::make_random_string<buffer>(1024);
        s.set_rx(data);

        const auto n = data.size();

        buffer buf(n);

        auto handler = [n](boost::system::error_code ec,
                           std::size_t bytes_transferred) {
            REQUIRE(!ec);
            REQUIRE(n == bytes_transferred);
        };

        s.async_read_some(asio::buffer(buf), handler);

        REQUIRE(data == buf);
    }
}

TEST_CASE("async_read", "[test][MockSock]")
{
    using buffer = std::vector<char>;
    using tcp_socket = test::MockSock<buffer, asio::io_context::executor_type>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

    {
        const auto data = test::make_random_string<buffer>(1024);
        s.set_rx(data);

        const auto n = data.size();

        buffer buf(n);

        auto handler = [n](boost::system::error_code ec,
                           std::size_t bytes_transferred) {
            REQUIRE(!ec);
            REQUIRE(n == bytes_transferred);
        };

        asio::async_read(s, asio::buffer(buf), handler);

        REQUIRE(data == buf);
    }
}

TEST_CASE("async_write_some", "[test][MockSock]")
{
    using buffer = std::vector<char>;
    using tcp_socket = test::MockSock<buffer, asio::io_context::executor_type>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

    {
        const auto data = test::make_random_string<buffer>(1024);

        const auto n = data.size();

        auto handler = [n](boost::system::error_code ec,
                           std::size_t bytes_transferred) {
            REQUIRE(!ec);
            REQUIRE(n == bytes_transferred);
        };

        s.async_write_some(asio::buffer(data), handler);

        REQUIRE(data == s.get_tx());
    }
}

TEST_CASE("async_write", "[test][MockSock]")
{
    using buffer = std::vector<char>;
    using tcp_socket = test::MockSock<buffer, asio::io_context::executor_type>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

    {
        const auto data = test::make_random_string<buffer>(1024);

        const auto n = data.size();

        auto handler = [n](boost::system::error_code ec,
                           std::size_t bytes_transferred) {
            REQUIRE(!ec);
            REQUIRE(n == bytes_transferred);
        };

        asio::async_write(s, asio::buffer(data), handler);

        REQUIRE(data == s.get_tx());
    }
}

TEST_CASE("session_ok", "[skye][session]")
{
    using buffer = std::string;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_socket = default_token::as_default_on_t<
        test::MockSock<buffer, asio::io_context::executor_type>>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

    const buffer data = "GET / HTTP/1.0\r\n\r\n";
    s.set_rx(data);

    const auto body = test::make_random_string<buffer>(1024);

    int handler_called = 0;
    auto handler = [&body, &handler_called](
                       skye::request req) -> asio::awaitable<skye::response> {
        ++handler_called;

        skye::response res(http::status::ok, req.version());
        res.body() = body;

        co_return res;
    };

    skye::SessionMetrics metrics;
    auto reporter = [&metrics](const skye::SessionMetrics& m) { metrics = m; };

    co_spawn(
        ctx.get_executor(), skye::session(s, handler, reporter),
        [](auto ptr) { REQUIRE(!ptr); });

    REQUIRE(ctx.run() > 0);

    REQUIRE(metrics.num_request == 1);
    REQUIRE(metrics.bytes_read == static_cast<int>(data.size()));

    REQUIRE(s.get_tx().ends_with(body));
    REQUIRE(handler_called == 1);
}

TEST_CASE("session_error", "[skye][session]")
{
    using buffer = std::string;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_socket = default_token::as_default_on_t<
        test::MockSock<buffer, asio::io_context::executor_type>>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};

    const buffer data = "GET / xxx HTTP/1.0\r\n\r\n";
    s.set_rx(data);

    bool handler_called = false;
    auto handler = [&handler_called](
                       skye::request req) -> asio::awaitable<skye::response> {
        handler_called = true;
        co_return skye::response{http::status::ok, req.version()};
    };

    skye::SessionMetrics metrics;
    auto reporter = [&metrics](const skye::SessionMetrics& m) { metrics = m; };

    co_spawn(ctx, skye::session(s, handler, reporter), [](auto ptr) {
        REQUIRE(!ptr);
    });

    REQUIRE(ctx.run() > 0);

    REQUIRE(metrics.num_request == 0);
    REQUIRE(!handler_called);
}

TEST_CASE("session_error_post", "[skye][session]")
{
    using buffer = std::string;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_socket = default_token::as_default_on_t<
        test::MockSock<buffer, asio::io_context::executor_type>>;

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};
    {
        const auto body = test::make_random_string<buffer>(1024 * 1024 + 1);
        const buffer data = "POST / HTTP/1.0\r\n"
                            "Content-Type: application/octet-stream\r\n"
                            "Content-Length: " +
                            std::to_string(body.size()) + "\r\n\r\n" + body;
        s.set_rx(data);
    }

    bool handler_called = false;
    auto handler = [&](skye::request req) -> asio::awaitable<skye::response> {
        handler_called = true;
        co_return skye::response{http::status::ok, req.version()};
    };

    bool reporter_called = false;
    skye::SessionMetrics metrics;
    auto reporter = [&](const skye::SessionMetrics& m) {
        reporter_called = true;
        metrics = m;
    };

    co_spawn(ctx, skye::session(s, handler, reporter), [](auto ptr) {
        REQUIRE(!ptr);
    });

    REQUIRE(ctx.run() > 0);

    REQUIRE(reporter_called);
    REQUIRE(metrics.num_request == 0);
    REQUIRE(!handler_called);
}