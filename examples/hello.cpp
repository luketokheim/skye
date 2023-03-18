#include <skye/service.hpp>

namespace asio = boost::asio;
namespace http = boost::beast::http;

asio::awaitable<skye::response> hello(skye::request req)
{
    skye::response res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"world\"}";

    co_return res;
}

int main()
{
    // Listen on 8080 or PORT env var.
    const int port = skye::getenv_port();

    // Listen on port and route all HTTP requests to hello. Installs a signal
    // handler for SIGINT and SIGTERM to cleanly stop the server.
    skye::run(port, hello);

    return 0;
}
