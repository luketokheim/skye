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
    try {
        // Listen on port 8080 and route all HTTP requests to the hello handler.
        // - Server and hello handler all run in main thread
        // - SIGINT and SIGTERM cleanly stop the server
        skye::run(8080, hello);

        return 0;
    } catch (...) {
    }

    return -1;
}
