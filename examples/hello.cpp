#include <httpmicroservice/service.hpp>

namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace usrv = httpmicroservice;

asio::awaitable<usrv::response> hello(usrv::request req)
{
    usrv::response res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"world\"}";

    co_return res;
}

int main()
{
    // Listen on 8080 or PORT env var.
    const int port = usrv::getenv_port();

    // Listen on port and route all HTTP requests to hello. Installs a signal
    // handler for SIGINT and SIGTERM to cleanly stop the server.
    usrv::run(port, hello);

    return 0;
}
