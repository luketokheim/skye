#include <httpmicroservice.hpp>
#include <httpmicroservice/service.hpp>

#include <boost/asio/signal_set.hpp>
#include <fmt/core.h>

#include <cstdio>
#include <exception>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;
namespace http = httpmicroservice::http;

asio::awaitable<usrv::response> handler(usrv::request req)
{
    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"world\"}";

    co_return res;
}

int main()
{
    try {
        auto port = usrv::getenv_port();

        auto ioc = asio::io_context{};

        usrv::async_run(ioc.get_executor(), port, handler, false);

        // SIGTERM is sent by Docker to ask us to stop (politely)
        // SIGINT handles local Ctrl+C in a terminal
        asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](auto ec, auto sig) { ioc.stop(); });

        ioc.run();

        return 0;
    } catch (std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        fmt::print(stderr, "unknown exception\n");
    }

    return -1;
}