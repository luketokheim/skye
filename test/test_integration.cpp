#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <httpmicroservice/service.hpp>

#include "test.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <future>
#include <string>

TEST_CASE("run_async")
{
    using namespace std::chrono_literals;
    namespace asio = boost::asio;
    namespace http = boost::beast::http;
    namespace usrv = httpmicroservice;
    using tcp = boost::asio::ip::tcp;

    constexpr auto Port = 8080;

    // Create a handler that returns random data in the body
    auto body = test::make_random_string<std::string>(1000);
    auto handler = [body](auto req) {
        usrv::response res(http::status::ok, req.version());
        res.set(http::field::content_type, "text/html");
        res.body() = body;
        return res;
    };

    // Run server in its own thread so we can call ctx.stop() from main
    asio::io_context ctx;
    auto server = std::async(std::launch::async, [&ctx, handler]() {
        usrv::async_run(ctx.get_executor(), Port, handler);

        ctx.run_for(5s);
    });

    usrv::request req(http::verb::get, "/", 11);

    // Run a HTTP client in its own thread, get http://127.0.0.1:8080/
    // https://github.com/boostorg/beast/blob/develop/example/http/client/sync/http_client_sync.cpp
    auto client = std::async(std::launch::async, [req]() {
        asio::io_context ctx;

        tcp::resolver resolver(ctx);
        auto endpoint = *resolver.resolve("0.0.0.0", std::to_string(Port));

        boost::system::error_code ec;

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
