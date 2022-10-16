#pragma once

#include <httpmicroservice/session.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <exception>

namespace httpmicroservice {

namespace asio = boost::asio;

template <typename Acceptor, typename Handler, typename Reporter>
asio::awaitable<void>
accept(Acceptor acceptor, Handler handler, Reporter reporter)
{
    using tcp = asio::ip::tcp;

    // constexpr auto report_stats =
    //    std::is_invocable_v<Reporter, const session_stats&>;

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
        if (reporter) {
            stats = std::make_optional<session_stats>();
        }

        // Run coroutine to handle one http connection
        co_spawn(
            acceptor.get_executor(),
            session(std::move(stream), handler, std::move(stats)),
            [&reporter](
                std::exception_ptr ptr, std::optional<session_stats> stats) {
                // Propagate exception from the coroutine
                if (ptr) {
                    std::rethrow_exception(ptr);
                }

                if (reporter && stats) {
                    reporter(*stats);
                }

                // if (stats) {
                //     fmt::print("{}\n", *stats);
                // }
            });
    }
}

template <typename Handler, typename Reporter>
asio::awaitable<void> listen(int port, Handler handler, Reporter reporter)
{
    using tcp = asio::ip::tcp;

    tcp::endpoint endpoint(tcp::v4(), port);

    tcp::acceptor acceptor(co_await asio::this_coro::executor, endpoint);

    co_await accept(
        std::move(acceptor), std::move(handler), std::move(reporter));
}

template <typename Executor, typename Handler, typename Reporter>
void async_run(Executor ex, int port, Handler handler, Reporter reporter)
{
    // Run coroutine to listen on our port
    co_spawn(
        ex, listen(port, std::move(handler), std::move(reporter)),
        [](std::exception_ptr ptr) {
            // Propagate exception from the coroutine
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        });
}

template <typename Executor, typename Handler>
std::function<asio::awaitable<response>(request)>
make_co_handler(Executor ex, Handler handler)
{
    return [ex, handler](request req) -> asio::awaitable<response> {
        auto res =
            co_await co_spawn(ex, handler(std::move(req)), asio::use_awaitable);
        co_return res;
    };
}

} // namespace httpmicroservice
