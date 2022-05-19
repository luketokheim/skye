#include <httpmicroservice.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <optional>

namespace httpmicroservice {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

constexpr auto kRequestSizeLimit = 1 << 20;

response_type make_response(request_type req)
{
    return response_type(http::status::ok, req.version());
}

struct session_stats {
    int fd = 0;
    int num_request = 0;
    int bytes_transferred = 0;
    std::chrono::steady_clock::duration duration;
};

std::ostream &operator<<(std::ostream &os, const session_stats &stats)
{
    os << "{\"fd\": " << stats.fd << "\"num_request\": " << stats.num_request
       << ", \"bytes_transferred\": " << stats.bytes_transferred
       << ", \"duration\": " << stats.duration << "}";
    return os;
}

asio::awaitable<std::optional<session_stats>> session(
    tcp::socket socket, handler_type handler,
    std::optional<session_stats> stats)
{
    std::chrono::steady_clock::time_point start_time;
    if (stats) {
        stats->fd = socket.native_handle();
        start_time = std::chrono::steady_clock::now();
    }

    boost::beast::flat_buffer buffer(kRequestSizeLimit);
    boost::system::error_code ec;

    for (;;) {
        // req = read(...)
        request_type req;
        {
            auto bytes_read = co_await http::async_read(
                socket, buffer, req,
                asio::redirect_error(asio::use_awaitable, ec));

            // if (ec == http::error::end_of_stream) {
            //     socket.shutdown(tcp::socket::shutdown_send, ec);
            //     break;
            // }

            if (ec) {
                break;
            }

            if (stats) {
                ++stats->num_request;
                stats->bytes_transferred += bytes_read;
            }
        }

        auto keep_alive = req.keep_alive();

        // res = handler(req)
        auto res = std::invoke(handler, std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        // write(res)
        auto bytes_write = co_await http::async_write(
            socket, res, asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            co_return stats;
        }

        if (stats) {
            stats->bytes_transferred += bytes_write;
        }

        if (res.need_eof()) {
            socket.shutdown(tcp::socket::shutdown_send, ec);
            break;
        }
    }

    if (stats) {
        stats->duration = std::chrono::steady_clock::now() - start_time;
    }

    co_return stats;
}

asio::awaitable<void> listen(int port, handler_type handler)
{
    tcp::endpoint endpoint(tcp::v4(), port);

    tcp::acceptor acceptor(co_await asio::this_coro::executor, endpoint);
    for (;;) {
        boost::system::error_code ec;

        auto socket = co_await acceptor.async_accept(
            asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            break;
        }

        // Run coroutine to handle one http connection
        co_spawn(
            acceptor.get_executor(), session(std::move(socket), handler, {}),
            [](auto ptr, auto stats) {
                // Propagate exception from the coroutine
                if (ptr) {
                    std::rethrow_exception(ptr);
                }

                if (stats) {
                    std::cout << *stats << "\n";
                }
            });
    }
}

int run(int port, handler_type handler)
{
    asio::io_context ctx;

    // Run coroutine to listen on our port
    co_spawn(ctx, listen(port, std::move(handler)), [](auto ptr) {
        // Propagate exception from the coroutine
        if (ptr) {
            std::rethrow_exception(ptr);
        }
    });

    ctx.run();

    return 0;
}

} // namespace httpmicroservice