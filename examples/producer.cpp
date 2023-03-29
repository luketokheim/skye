#include <boost/asio/signal_set.hpp>
#include <boost/asio/thread_pool.hpp>
#include <fmt/core.h>
#include <skye/format.hpp>
#include <skye/service.hpp>
#include <skye/utility.hpp>

#include <chrono>
#include <cstdio>
#include <exception>
#include <thread>

namespace asio = boost::asio;
namespace http = boost::beast::http;

// Runs on the pool executor which runs in its own thread.
asio::awaitable<skye::response> producer(skye::request req)
{
    using namespace std::chrono_literals;

    constexpr auto kSimulatedDelay = 250ms;

    // If your handler might take some time and does not support async then
    // you should use your own thread so that the server can keep handling
    // requests.
    std::this_thread::sleep_for(kSimulatedDelay);

    skye::response res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"producer\"}";

    co_return res;
}

// Print aggregate session metrics to stdout. Called once per socket connection
// which likely includes multiple HTTP requests.
void reporter(const skye::session_metrics& metrics)
{
    fmt::print("{}\n", metrics);
}

int main()
{
    try {
        const int port = skye::getenv_port();

        asio::io_context ioc{1};
        asio::thread_pool pool{1};

        // Two threads. Main thread runs the http service and the I/O to read
        // requests and write responses. The pool thread runs the producer
        // function.
        // Warning! This is just an example of how you to run one thread for
        // some blocking operation. Not tested in your specific app and you may
        // find that the single threaded option performs better at the cost of
        // blocking the event loop.
        skye::async_run(
            ioc, port, skye::make_co_handler(pool, producer), reporter);

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
