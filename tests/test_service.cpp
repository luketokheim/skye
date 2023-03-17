#include <skye/service.hpp>

#include <catch2/catch_test_macros.hpp>

#include "test.hpp"

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <future>
#include <string>
#include <thread>

namespace asio = boost::asio;
namespace http = boost::beast::http;

TEST_CASE("async_run", "[service]")
{
    using namespace std::chrono_literals;
    using tcp = boost::asio::ip::tcp;

    constexpr auto kBodyNumBytes = 1000;
    constexpr auto kPort = 8080;
    constexpr auto kHttpVersion = 11;

    // Create a handler that returns random data in the body
    auto body = test::make_random_string<std::string>(kBodyNumBytes);
    auto handler = [body](auto req) {
        skye::response res(http::status::ok, req.version());
        res.set(http::field::content_type, "text/html");
        res.body() = body;
        return res;
    };

    auto awaitable_handler =
        [handler](auto req) -> asio::awaitable<skye::response> {
        auto res = handler(std::move(req));
        co_return res;
    };

    auto reporter = [](const skye::session_metrics& metrics) {
        REQUIRE(metrics.fd > 0);

        if (metrics.num_request > 0) {
            REQUIRE(metrics.bytes_read > 0);
            REQUIRE(metrics.bytes_write > 0);
            REQUIRE(metrics.end_time > metrics.start_time);
        }
    };

    // Run server in its own thread so we can call ioc.stop() from main
    asio::io_context ioc;
    auto server =
        std::async(std::launch::async, [&ioc, awaitable_handler, reporter]() {
            skye::async_run(ioc, kPort, awaitable_handler, reporter);

            ioc.run_for(5s);
        });

    skye::request req(http::verb::get, "/", kHttpVersion);

    // Run a HTTP client in its own thread, get http://localhost:8080/
    // https://github.com/boostorg/beast/blob/develop/example/http/client/sync/http_client_sync.cpp
    auto client = std::async(std::launch::async, [req]() {
        asio::io_context ioc;

        const tcp::endpoint endpoint{
            asio::ip::make_address("127.0.0.1"),
            static_cast<asio::ip::port_type>(kPort)};

        boost::system::error_code ec;

        // Just try to connect
        for (int i = 0; i < 5; ++i) {
            tcp::socket socket(ioc);
            socket.connect(endpoint, ec);
            if (!ec) {
                break;
            }

            std::this_thread::sleep_for(100ms);
        }

        boost::beast::tcp_stream stream(ioc);
        stream.expires_after(2s);
        stream.connect(endpoint);

        http::write(stream, req);

        boost::beast::flat_buffer buffer;

        skye::response res;
        http::read(stream, buffer, res, ec);

        stream.socket().shutdown(tcp::socket::shutdown_both);

        return res;
    });

    REQUIRE(client.wait_for(2s) == std::future_status::ready);

    REQUIRE_NOTHROW([&]() {
        auto response = client.get();
        auto expected = handler(req);

        REQUIRE(response.body() == expected.body());
    }());

    ioc.stop();

    REQUIRE(server.wait_for(2s) == std::future_status::ready);
    REQUIRE_NOTHROW(server.get());
}

TEST_CASE("make_co_handler", "[service]")
{
    asio::io_context ioc;

    const auto tid = std::this_thread::get_id();

    auto awaitable_handler =
        [tid](skye::request req) -> asio::awaitable<skye::response> {
        REQUIRE(tid != std::this_thread::get_id());

        throw std::exception{};
        co_return skye::response{};
    };

    asio::thread_pool pool{1};

    auto co_handler = skye::make_co_handler(pool, awaitable_handler);

    co_spawn(
        ioc,
        [co_handler]() -> asio::awaitable<void> {
            skye::request req(skye::http::verb::get, "/", 11);
            co_await co_handler(req);
        },
        [](auto ptr) {
            REQUIRE(ptr);
            std::rethrow_exception(ptr);
        });

    REQUIRE_THROWS(ioc.run());
}

TEST_CASE("async_run_functor", "[service]")
{
    constexpr auto kPort = 8080;

    struct Handler {
        asio::awaitable<skye::response> operator()(skye::request req) const
        {
            co_return skye::response{http::status::ok, req.version()};
        }

        std::shared_ptr<int> ctx;
    };

    // Handler function object. One use case is to include some shared state or
    // context. This could be some other resource handle to DB, etc.
    Handler handler;
    handler.ctx = std::make_shared<int>(100);

    asio::io_context ioc;

    skye::async_run(ioc, kPort, handler);

    REQUIRE(ioc.run_one() > 0);
}

TEST_CASE("async_run_context", "[service]")
{
    constexpr auto kPort = 8080;

    auto handler = [](auto req) -> asio::awaitable<skye::response> {
        co_return skye::response(http::status::ok, req.version());
    };

    auto server = std::async(std::launch::async, [handler]() {
        asio::io_context ioc;
        skye::async_run(ioc, kPort, handler);

        REQUIRE(ioc.run_one() > 0);
    });
}

TEST_CASE("integration", "[service]")
{
    using namespace std::chrono_literals;
    using tcp = boost::asio::ip::tcp;

    constexpr auto kPort = 8081;

    auto handler = [](skye::request req) -> asio::awaitable<skye::response> {
        co_return skye::response{http::status::ok, req.version()};
    };

    auto reporter = [](const skye::session_metrics& metrics) {
        REQUIRE(metrics.fd > 0);

        if (metrics.num_request > 0) {
            REQUIRE(metrics.bytes_read > 0);
            REQUIRE(metrics.bytes_write > 0);
            REQUIRE(metrics.end_time > metrics.start_time);
        }
    };

    asio::io_context ioc;

    skye::async_run(ioc, kPort, handler, reporter);

    int num_client = 0;

    auto client_partial_write = [&num_client]() -> asio::awaitable<void> {
        // Send partial request and close socket
        {
            const tcp::endpoint endpoint{
                asio::ip::make_address("127.0.0.1"),
                static_cast<asio::ip::port_type>(kPort)};

            tcp::socket socket{co_await asio::this_coro::executor};

            boost::system::error_code ec;
            for (int i = 0; i < 5; ++i) {
                co_await socket.async_connect(
                    endpoint, asio::redirect_error(asio::use_awaitable, ec));
                if (!ec) {
                    break;
                }

                asio::steady_timer timer{socket.get_executor(), 100ms};
                co_await timer.async_wait(asio::use_awaitable);
            }

            REQUIRE(!ec);

            co_await asio::async_write(
                socket, asio::buffer("POST /target HT"),
                asio::redirect_error(asio::use_awaitable, ec));

            REQUIRE(!ec);

            ++num_client;
        }
    };

    auto client_partial_read = [&num_client]() -> asio::awaitable<void> {
        // Send complete request, do partial read
        {
            const tcp::endpoint endpoint{
                asio::ip::make_address("127.0.0.1"),
                static_cast<asio::ip::port_type>(kPort)};

            tcp::socket socket{co_await asio::this_coro::executor};

            boost::system::error_code ec;
            for (int i = 0; i < 5; ++i) {
                co_await socket.async_connect(
                    endpoint, asio::redirect_error(asio::use_awaitable, ec));
                if (!ec) {
                    break;
                }

                asio::steady_timer timer{socket.get_executor(), 100ms};
                co_await timer.async_wait(asio::use_awaitable);
            }

            REQUIRE(!ec);

            co_await asio::async_write(
                socket, asio::buffer("GET / HTTP/1.1\r\n\r\n"),
                asio::redirect_error(asio::use_awaitable, ec));

            REQUIRE(!ec);

            std::array<char, 4> buf{};
            co_await asio::async_read(
                socket, asio::buffer(buf),
                asio::redirect_error(asio::use_awaitable, ec));

            REQUIRE(!ec);

            ++num_client;
        }

        co_return;
    };

    auto coro = [&]() -> asio::awaitable<void> {
        using namespace asio::experimental::awaitable_operators;

        co_await (client_partial_read() && client_partial_write());
    };

    co_spawn(ioc, coro(), [&ioc](auto ptr) {
        REQUIRE(!ptr);
        ioc.stop();
    });

    REQUIRE(ioc.run() > 0);
    REQUIRE(num_client == 2);
}
