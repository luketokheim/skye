#pragma once

#include <httpmicroservice/session.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <exception>
#include <type_traits>

namespace httpmicroservice {

namespace asio = boost::asio;

/**
  The service connection accept loop. Launch a coroutine for each incoming
  socket stream connection.

  loop {
      stream = accept()

      // One coroutine per session. When it is complete, call the Reporter
      // with the session stats.
      stats = await session(stream)
  }
 */
template <typename Acceptor, typename Handler, typename Reporter>
asio::awaitable<void>
accept(Acceptor acceptor, Handler handler, Reporter reporter)
{
    using tcp = asio::ip::tcp;

    constexpr bool kEnableStats =
        std::is_invocable_v<Reporter, const session_stats&>;

    for (;;) {
        boost::system::error_code ec;

        auto stream = co_await acceptor.async_accept(
            asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            break;
        }

        stream.set_option(tcp::no_delay{true}, ec);

        if (ec) {
            continue;
        }

        std::optional<session_stats> stats;
        if constexpr (kEnableStats) {
            stats = std::make_optional<session_stats>();
        }

        // Run coroutine to handle one http connection
        co_spawn(
            acceptor.get_executor(),
            session(std::move(stream), handler, std::move(stats)),
            [&reporter](auto ptr, auto stats) {
                // Propagate exception from the coroutine
                if (ptr) {
                    std::rethrow_exception(ptr);
                }

                if constexpr (kEnableStats) {
                    if (stats) {
                        reporter(*stats);
                    }
                }
            });
    }
}

/**
  Bind and listen for incoming connections on the specified port on all
  IP addresses.
 */
template <typename Handler, typename Reporter>
asio::awaitable<void> listen(int port, Handler handler, Reporter reporter)
{
    using tcp = asio::ip::tcp;

    tcp::endpoint endpoint(tcp::v4(), port);

    tcp::acceptor acceptor(co_await asio::this_coro::executor, endpoint);

    co_await accept(
        std::move(acceptor), std::move(handler), std::move(reporter));
}

/**
  Run the server in a coroutine. Convenient to call similar to asio::async_read
  style free functions.
 */
template <typename Executor, typename Handler, typename Reporter>
void async_run(Executor ex, int port, Handler handler, Reporter reporter)
{
    // Run coroutine to listen on our port
    co_spawn(
        ex, listen(port, std::move(handler), std::move(reporter)),
        [](auto ptr) {
            // Propagate exception from the coroutine
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        });
}

template <typename ExecutionContext, typename Handler>
void async_run(ExecutionContext& context, int port, Handler handler)
{
    async_run(context.get_executor(), port, std::move(handler), false);
}

/**
  Wrap a HTTP request handler in its own coroutine. Intended for use with a
  different Executor not running in the main I/O thread. This is the mechanism
  to use asio::thread_pool::get_executor to run the handlers outside the service
  I/O thread.
 */
template <typename Executor, typename Handler>
auto make_co_handler(Executor ex, Handler handler)
{
    return [ex, handler](request req) -> asio::awaitable<response> {
        auto res =
            co_await co_spawn(ex, handler(std::move(req)), asio::use_awaitable);
        co_return res;
    };
}

/**
  Read the PORT environment variable. Returns 8080 if the PORT variable is not
  set or is not an integer between 1024 and 65535.

  Cloud Run sets the PORT environment variable.
  https://cloud.google.com/run/docs/container-contract#port
 */
int getenv_port();

} // namespace httpmicroservice
