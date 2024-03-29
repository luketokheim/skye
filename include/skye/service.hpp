//
// skye/service.hpp
//
// Copyright 2023 Luke Tokheim
//
/**
  Entry point for a skye HTTP server. User calls run or async_run with their
  handler function.

  Usage:

  // A handler is a function object that takes a request and returns a response.
  auto handler = [](request req) -> asio::awaitable<response> {
    response res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Hello World!";

    co_return res;
  };

  // Listen on port 8080 and route all HTTP requests to the handler.
  run(8080, handler);
*/
#ifndef SKYE_SERVICE_HPP_
#define SKYE_SERVICE_HPP_

#include <skye/session.hpp>
#include <skye/types.hpp>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <exception>

namespace skye {

namespace asio = boost::asio;

namespace detail {

/**
  The service connection accept loop. Spawn a coroutine for each incoming socket
  stream connection.

  loop {
    // Incoming socket connection
    stream = accept()

    // HTTP request/response loop
    co_spawn session(stream)
  }
*/
asio::awaitable<void>
accept(auto acceptor, Handler auto handler, Reporter auto reporter)
{
    using tcp = asio::ip::tcp;

    for (;;) {
        auto [ec, stream] = co_await acceptor.async_accept();

        if (ec) {
            continue;
        }

        stream.set_option(tcp::no_delay{true}, ec);

        if (ec) {
            continue;
        }

        // Run coroutine to handle one http connection
        co_spawn(
            acceptor.get_executor(),
            session(std::move(stream), handler, reporter), [](auto ptr) {
                // Propagate exception from the coroutine
                if (ptr) {
                    std::rethrow_exception(ptr);
                }
            });
    }
}

/**
  Bind and listen for incoming connections on the specified port on all
  IP addresses.
*/
asio::awaitable<void>
listen(int port, Handler auto handler, Reporter auto reporter)
{
    // Use a custom completion token for async operations on the acceptor and
    // its incoming socket connections.
    // - Always use co_await
    // - Always return error_code and result as a std::tuple
    using tcp = asio::ip::tcp;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;

    tcp::endpoint endpoint{tcp::v4(), static_cast<asio::ip::port_type>(port)};

    tcp_acceptor acceptor{co_await asio::this_coro::executor, endpoint};

    co_await accept(
        std::move(acceptor), std::move(handler), std::move(reporter));
}

} // namespace detail

/**
  Run the server in a coroutine. Convenient to call similar to asio::async_read
  style free functions.

  The handler function object is called once per request.

  The optional reporter function object is called once per socket session which
  may span multiple requests.
*/
template <typename ExecutionContext, Handler Handler, Reporter Reporter = bool>
void async_run(
    ExecutionContext& ctx, int port, Handler handler, Reporter reporter = {})
{
    // Run coroutine to listen on our port
    co_spawn(
        ctx, detail::listen(port, std::move(handler), std::move(reporter)),
        [](auto ptr) {
            // Propagate exception from the coroutine
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        });
}

/**
  Run a server. Listen on port and route all requests to the handler function
  object.

  Run event loop "forever" on this thread. Sets a signal handler to stop
  cleanly.

  Opinionated design for use in a container behind load balancer. Listen on
  port until the container runtime sends a SIGTERM signal. Single thread, scale
  service horizontally with more instances.

  The optional reporter function object is called once per socket session which
  may span multiple requests.
*/
template <Handler Handler, Reporter Reporter = bool>
void run(int port, Handler handler, Reporter reporter = {})
{
    // Concurrency hint to asio that run is single threaded
    asio::io_context ioc{1};

    // Listen on port and route all HTTP requests to the handler
    async_run(ioc, port, std::move(handler), std::move(reporter));

    // SIGTERM is sent by Docker to ask us to stop (politely)
    // SIGINT handles local Ctrl+C in a terminal
    asio::signal_set signals{ioc, SIGINT, SIGTERM};
    signals.async_wait([&ioc](auto /*ec*/, auto /*sig*/) { ioc.stop(); });

    // Run event processing loop
    ioc.run();
}

/**
  Wrap a HTTP request handler in its own coroutine. Intended for use with a
  second ExecutionContext not running in the main I/O thread. This is the
  mechanism to use asio::thread_pool to run the handlers separately from the
  main server event loop.
*/
template <typename ExecutionContext, Handler Handler>
auto make_co_handler(ExecutionContext& ctx, Handler handler)
{
    auto ex = ctx.get_executor();
    return [=](request req) -> asio::awaitable<response> {
        return co_spawn(ex, handler(std::move(req)), asio::use_awaitable);
    };
}

} // namespace skye

#endif // SKYE_SERVICE_HPP_
