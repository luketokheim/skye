#include <httpmicroservice/service.hpp>

#include <catch2/catch_test_macros.hpp>

#include "test.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <future>
#include <string>
#include <thread>

TEST_CASE("async_run", "[service]")
{
    using namespace std::chrono_literals;
    namespace asio = boost::asio;
    namespace http = boost::beast::http;
    namespace usrv = httpmicroservice;
    using tcp = boost::asio::ip::tcp;

    constexpr auto kBodyNumBytes = 1000;
    constexpr auto kPort = 8080;
    constexpr auto kHttpVersion = 11;

    // Create a handler that returns random data in the body
    auto body = test::make_random_string<std::string>(kBodyNumBytes);
    auto handler = [body](auto req) {
        usrv::response res(http::status::ok, req.version());
        res.set(http::field::content_type, "text/html");
        res.body() = body;
        return res;
    };

    auto awaitable_handler =
        [handler](auto req) -> asio::awaitable<usrv::response> {
        auto res = handler(std::move(req));
        co_return res;
    };

    auto reporter = [](const usrv::session_stats& stats) {
        REQUIRE(stats.fd > 0);

        if (stats.num_request > 0) {
            REQUIRE(stats.bytes_read > 0);
            REQUIRE(stats.bytes_write > 0);
            REQUIRE(stats.end_time > stats.start_time);
        }
    };

    // Run server in its own thread so we can call ctx.stop() from main
    asio::io_context ctx;
    auto server =
        std::async(std::launch::async, [&ctx, awaitable_handler, reporter]() {
            usrv::async_run(
                ctx.get_executor(), kPort, awaitable_handler, reporter);

            ctx.run_for(5s);
        });

    usrv::request req(http::verb::get, "/", kHttpVersion);

    // Run a HTTP client in its own thread, get http://localhost:8080/
    // https://github.com/boostorg/beast/blob/develop/example/http/client/sync/http_client_sync.cpp
    auto client = std::async(std::launch::async, [req]() {
        asio::io_context ctx;

        tcp::resolver resolver(ctx);
        const tcp::endpoint endpoint =
            *resolver.resolve("127.0.0.1", std::to_string(kPort));

        boost::system::error_code ec;

        // Just try to connect
        for (int i = 0; i < 5; ++i) {
            tcp::socket socket(ctx);
            socket.connect(endpoint, ec);
            if (!ec) {
                break;
            }

            std::this_thread::sleep_for(100ms);
        }

        boost::beast::tcp_stream stream(ctx);
        stream.expires_after(2s);
        stream.connect(endpoint);

        http::write(stream, req);

        boost::beast::flat_buffer buffer;

        usrv::response res;
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

    ctx.stop();

    REQUIRE(server.wait_for(2s) == std::future_status::ready);
    REQUIRE_NOTHROW(server.get());
}

TEST_CASE("make_co_handler", "[service]")
{
    namespace asio = boost::asio;
    namespace usrv = httpmicroservice;

    asio::io_context ioc;

    const auto tid = std::this_thread::get_id();

    auto awaitable_handler =
        [tid](usrv::request req) -> asio::awaitable<usrv::response> {
        REQUIRE(tid != std::this_thread::get_id());

        throw std::exception{};
        co_return usrv::response{};
    };

    asio::thread_pool pool{1};

    auto co_handler =
        usrv::make_co_handler(pool.get_executor(), awaitable_handler);

    co_spawn(
        ioc,
        [co_handler]() -> asio::awaitable<void> {
            usrv::request req(usrv::http::verb::get, "/", 11);
            co_await co_handler(req);
        },
        [](auto ptr) {
            REQUIRE(ptr);
            std::rethrow_exception(ptr);
        });

    REQUIRE_THROWS(ioc.run());
}