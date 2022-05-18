#include <httpappserver.hpp>

/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

// #include <chrono>
#include <exception>

namespace httpappserver {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

response_type make_response(request_type req)
{
    response_type res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/html");
    res.body() = "Hello World!";

    return res;
}

#if 0

asio::awaitable<void> session(tcp::socket socket)
{
    beast::flat_buffer buffer;
    for (;;) {
        http::request<http::string_body> req;
        co_await http::async_read(socket, buffer, req, asio::use_awaitable);

        const bool keep_alive = req.keep_alive();

        auto res = make_response(std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        co_await http::async_write(socket, res, asio::use_awaitable);

        if (res.need_eof()) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send);
}

#else

asio::awaitable<void> session(tcp::socket socket, handler_type handler)
{
    namespace beast = boost::beast;
    // using namespace std::chrono_literals;

    constexpr auto RequestSizeLimit = 1 << 20;

    beast::flat_buffer buffer(RequestSizeLimit);
    beast::error_code ec;

    for (;;) {
        // req = read(...)
        http::request<http::string_body> req;
        co_await http::async_read(
            socket, buffer, req, asio::redirect_error(asio::use_awaitable, ec));

        if (ec == http::error::end_of_stream) {
            break;
        }

        if (ec) {
            co_return;
        }

        const bool keep_alive = req.keep_alive();

        // res = handler(req)
        auto res = std::invoke(handler, std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        // write(res)
        co_await http::async_write(
            socket, res, asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            co_return;
        }

        if (res.need_eof()) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send, ec);
}

#endif

asio::awaitable<void> listen(tcp::endpoint endpoint, handler_type handler)
{
    tcp::acceptor acceptor{co_await asio::this_coro::executor, endpoint};
    for (;;) {
        boost::system::error_code ec;
        auto socket = co_await acceptor.async_accept(
            asio::redirect_error(asio::use_awaitable, ec));
        if (ec) {
            break;
        }

        co_spawn(
            acceptor.get_executor(), session(std::move(socket), handler),
            asio::detached);
    }
}

int run(std::string_view host, std::string_view port, handler_type handler)
{
    asio::io_context ctx;

    auto endpoint =
        *tcp::resolver(ctx).resolve(host, port, tcp::resolver::passive);

    // Run coroutine to listen on our host:port endpoint
    co_spawn(ctx, listen(std::move(endpoint), std::move(handler)), [&ctx](auto ptr) {
        // Propagate exception from the coroutine
        if (ptr) {
            std::rethrow_exception(ptr);
        }

        // The listen loop ended, call stop to end the signal handler
        ctx.stop();
    });

    // Install a signal handler
    // SIGINT to handle Ctrl+C
    // SIGTERM to handle shutdown request from "docker stop" or from Cloud Run
    // https://docs.docker.com/engine/reference/commandline/stop/
    // https://cloud.google.com/run/docs/container-contract#instance-shutdown
    asio::signal_set signals{ctx, SIGINT, SIGTERM};
    signals.async_wait([&ctx](auto ec, int signo) { ctx.stop(); });

    ctx.run();

    return 0;
}

} // namespace httpappserver