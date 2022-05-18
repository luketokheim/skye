#include <httpmicroservice.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <exception>

namespace httpmicroservice {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

constexpr auto kRequestSizeLimit = 1 << 20;

response_type make_response(request_type req)
{
    return response_type(http::status::ok, req.version());
}

asio::awaitable<void> session(tcp::socket socket, handler_type handler)
{
    boost::beast::flat_buffer buffer(kRequestSizeLimit);
    boost::system::error_code ec;

    for (;;) {
        // req = read(...)
        request_type req;
        co_await http::async_read(
            socket, buffer, req, asio::redirect_error(asio::use_awaitable, ec));

        if (ec == http::error::end_of_stream) {
            break;
        }

        if (ec) {
            co_return;
        }

        auto keep_alive = req.keep_alive();

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

void rethrow_exception_ptr(std::exception_ptr ptr)
{
    // Propagate exception from a coroutine
    if (ptr) {
        std::rethrow_exception(ptr);
    }
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
            acceptor.get_executor(), session(std::move(socket), handler),
            rethrow_exception_ptr);
    }
}

int run(int port, handler_type handler)
{
    asio::io_context ctx;

    // Run coroutine to listen on our port
    co_spawn(ctx, listen(port, std::move(handler)), rethrow_exception_ptr);

    ctx.run();

    return 0;
}

} // namespace httpmicroservice