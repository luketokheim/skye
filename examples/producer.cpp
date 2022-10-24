#include <httpmicroservice/format.hpp>
#include <httpmicroservice/service.hpp>

#include <boost/asio/signal_set.hpp>
#include <boost/asio/thread_pool.hpp>
#include <fmt/core.h>

#include <chrono>
#include <cstdio>
#include <exception>
#include <thread>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;
namespace http = boost::beast::http;

// Runs on the pool executor which runs in its own thread.
asio::awaitable<usrv::response> producer(usrv::request req)
{
    using namespace std::chrono_literals;

    // If your handler might take some time and does not support async then
    // you should use your own thread so that the server can keep handling
    // requests.
    std::this_thread::sleep_for(100ms);

    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"producer\"}";

    co_return res;
}

// Print aggregate session stats to stdout. Called once per socket connection
// which likely includes multiple HTTP requests.
void reporter(const usrv::session_stats& stats)
{
    fmt::print("{}\n", stats);
}

int main()
{
    try {
        const auto port = usrv::getenv_port();

        auto ioc = asio::io_context{};

        asio::thread_pool pool{1};
        auto ex = pool.get_executor();

        // Two threads. Main thread runs the http service and the I/O to read
        // requests and write responses. The pool thread runs the producer
        // function.
        usrv::async_run(
            ioc.get_executor(), port, usrv::make_co_handler(ex, producer),
            reporter);

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
