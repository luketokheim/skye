#include <httpappserver.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <exception>
#include <string_view>

namespace httpappserver {

namespace beast = boost::beast;
namespace http = beast::http;

asio::awaitable<void> session(tcp::socket socket)
{
    http::request<http::string_body> req;
    {
        beast::flat_buffer buffer;
        co_await http::async_read(socket, buffer, req, asio::use_awaitable);
    }

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "Hello World!";
    res.prepare_payload();

    http::response_serializer<http::string_body> serializer{res};

    co_await http::async_write(socket, serializer, asio::use_awaitable);
}

asio::awaitable<void> listen(tcp::endpoint endpoint)
{
    tcp::acceptor acceptor{co_await asio::this_coro::executor, endpoint};
    for (;;) {
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);

        co_spawn(
            acceptor.get_executor(), session(std::move(socket)),
            asio::detached);
    }
}

int run()
{
    constexpr std::string_view host = "0.0.0.0";
    constexpr std::string_view service = "http";

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