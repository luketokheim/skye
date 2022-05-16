#include <httpappserver.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <exception>

namespace httpappserver {

namespace beast = boost::beast;
namespace http = beast::http;

http::response<http::string_body>
make_response(http::request<http::string_body> &&req)
{
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/html");
    res.body() = "Hello World!";

    return res;
}

asio::awaitable<void> session(tcp::socket socket)
{
    for (;;) {
        http::request<http::string_body> req;
        {
            beast::flat_buffer buffer;
            co_await http::async_read(socket, buffer, req, asio::use_awaitable);
        }

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

asio::awaitable<void> session_ec(tcp::socket socket)
{
    beast::error_code ec;
    beast::flat_buffer buffer;

    for (;;) {
        http::request<http::string_body> req;
        {
            co_await http::async_read(
                socket, buffer, req,
                asio::redirect_error(asio::use_awaitable, ec));

            if (ec == http::error::end_of_stream) {
                break;
            }

            if (ec) {
                co_return;
            }
        }

        const bool keep_alive = req.keep_alive();

        auto res = make_response(std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

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

asio::awaitable<void> listen(tcp::endpoint endpoint)
{
    tcp::acceptor acceptor{co_await asio::this_coro::executor, endpoint};
    for (;;) {
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);

        co_spawn(
            acceptor.get_executor(), session_ec(std::move(socket)),
            asio::detached);
    }
}

int run(std::string_view host, std::string_view service)
{
    asio::io_context ctx;

    auto endpoint =
        *tcp::resolver(ctx).resolve(host, service, tcp::resolver::passive);

    co_spawn(ctx, listen(std::move(endpoint)), [](auto ptr) {
        // Propagate exception from the coroutine
        if (ptr) {
            std::rethrow_exception(ptr);
        }
    });

    ctx.run();

    return 0;
}

} // namespace httpappserver