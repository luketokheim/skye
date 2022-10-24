#include <httpmicroservice/service.hpp>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/signal_set.hpp>
#include <fmt/core.h>

#include <cstdio>
#include <exception>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;
namespace http = httpmicroservice::http;

// Handle POST requests, echo back the body contents
usrv::response post(const usrv::request& req)
{
    // Echo the POST request body
    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "text/plain");
    res.body() = req.body();

    // Apply tranformations based on the target
    if (req.target() == "/reverse") {
        std::reverse(res.body().begin(), res.body().end());
    } else if (req.target() == "/uppercase") {
        boost::algorithm::to_upper(res.body());
    } else if (req.target() == "/lowercase") {
        boost::algorithm::to_lower(res.body());
    } else if (req.target() == "/yell") {
        res.body().append("!!");
    }

    return res;
}

// Handle GET requests, echo back the target path
usrv::response get(const usrv::request& req)
{
    // This is how to to respond with a "404 Not Found"
    if (req.target() == "/not_found") {
        return usrv::response(http::status::not_found, req.version());
    }

    std::string_view target{req.target().data(), req.target().size()};

    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "application/json");
    res.body() = fmt::format("{{\"hello\": \"{}\"}}", target);

    return res;
}

// Route requests based on the method GET or POST
asio::awaitable<usrv::response> route(usrv::request req)
{
    switch (req.method()) {
    case http::verb::get:
        co_return get(req);
        break;
    case http::verb::post:
        co_return post(req);
        break;
    default:
        co_return usrv::response(
            http::status::method_not_allowed, req.version());
    }
}

int main()
{
    try {
        const int port = usrv::getenv_port();

        asio::io_context ioc;

        usrv::async_run(ioc, port, route);

        // SIGTERM is sent by Docker to ask us to stop (politely)
        // SIGINT handles local Ctrl+C in a terminal
        asio::signal_set signals{ioc, SIGINT, SIGTERM};
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