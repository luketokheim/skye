#include <httpappserver.hpp>

/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/lexical_cast.hpp>

#include <array>
#include <chrono>
#include <exception>
#include <future>
#include <thread>
#include <vector>

namespace httpappserver {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

http::response<http::string_body>
make_response(http::request<http::string_body> &&req)
{
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/html");
    res.body() = "Hello World!";

    return res;
}

asio::awaitable<void> session(tcp::socket socket)
{
    for (;;) {
        http::request<http::string_body> req;
        {
            beast::flat_buffer buffer;
            co_await http::async_read(socket, buffer, req, asio::use_awaitable);
        }

        const bool keep_alive = req.keep_alive();

        auto res = make_response(std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        co_await http::async_write(socket, res, asio::use_awaitable);

        if (res.need_eof()) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send);
}

void sync_session(tcp::socket &&socket)
{
    beast::error_code ec;

    for (;;) {
        http::request<http::string_body> req;
        {
            beast::flat_buffer buffer;
            http::read(socket, buffer, req, ec);

            if (ec == http::error::end_of_stream) {
                break;
            }

            if (ec) {
                return;
            }
        }

        const bool keep_alive = req.keep_alive();

        auto res = make_response(std::move(req));
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.prepare_payload();
        res.keep_alive(keep_alive);

        http::write(socket, res, ec);

        if (ec) {
            return;
        }

        if (res.need_eof()) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send, ec);
}

asio::awaitable<void> session_ec(tcp::socket socket)
{
    beast::error_code ec;

    for (;;) {
        http::request<http::string_body> req;
        {
            beast::flat_buffer buffer;
            co_await http::async_read(
                socket, buffer, req,
                asio::redirect_error(asio::use_awaitable, ec));

            if (ec == http::error::end_of_stream) {
                break;
            }

            if (ec) {
                co_return;
            }
        }

        const bool keep_alive = req.keep_alive();

        auto res = make_response(std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        co_await http::async_write(
            socket, res, asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            co_return;
        }

        if (res.need_eof()) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send, ec);
}

asio::awaitable<void> listen(tcp::endpoint endpoint)
{
    tcp::acceptor acceptor{co_await asio::this_coro::executor, endpoint};
    for (;;) {
        beast::error_code ec;
        auto socket = co_await acceptor.async_accept(
            asio::redirect_error(asio::use_awaitable, ec));
        if (ec) {
            break;
        }

        co_spawn(
            acceptor.get_executor(), session_ec(std::move(socket)),
            asio::detached);
    }
}

asio::awaitable<void> watchdog()
{
    asio::signal_set signals{
        co_await asio::this_coro::executor, SIGINT, SIGTERM};
    co_await signals.async_wait(asio::use_awaitable);
}

asio::awaitable<void> run_server(tcp::endpoint endpoint)
{
    using namespace asio::experimental::awaitable_operators;

    co_await(listen(std::move(endpoint)) || watchdog());
}

int run(std::string_view host, std::string_view port)
{
    asio::io_context ctx;

#if 0

    {
        auto endpoint =
            *tcp::resolver(ctx).resolve(host, port, tcp::resolver::passive);

        co_spawn(ctx, run_server(std::move(endpoint)), [](auto ptr) {
            // Propagate exception from the coroutine
            if (ptr) {
                std::rethrow_exception(ptr);
            }
        });
    }

    std::vector<std::jthread> thread_list;
    {
        const auto num_thread = std::thread::hardware_concurrency();
        if (num_thread > 1) {
            thread_list.resize(num_thread - 1);

            std::generate(thread_list.begin(), thread_list.end(), [&ctx]() {
                return std::jthread([&ctx]() { ctx.run(); });
            });
        }
    }

#else
    using namespace std::chrono_literals;

    auto cleanup = [](auto pool, auto duration) {
        std::vector<std::future<void>> busy;
        for (auto &future : pool) {
            if (future.wait_for(duration) == std::future_status::ready) {
                future.get();
            } else {
                busy.push_back(std::move(future));
            }
        }

        return busy;
    };

    auto endpoint =
        *tcp::resolver(ctx).resolve(host, port, tcp::resolver::passive);

    tcp::acceptor acceptor{ctx, endpoint};

    std::vector<std::future<void>> pool;
    //std::array<std::future<void>, 2> pool;
   //auto last = pool.begin();

    for (;;) {
        auto socket = acceptor.accept();

        auto future =
            std::async(std::launch::async, sync_session, std::move(socket));

        if (pool.size() >= 80) {
            pool = cleanup(std::move(pool), 0ms);

            while (pool.size() >= 160) {
                pool = cleanup(std::move(pool), 1s);
            }
        }

        pool.push_back(std::move(future));

        /*
        if (last == pool.end()) {
            for (auto &item : pool) {
                if (item.wait_for(1s) != std::future_status::ready) {
                    continue;
                } 

                item.get();

                std::swap(item, future);
                break;
            }
        } else {
            *last++ = std::move(future);
        }
        */
        /*
        if (last - pool.begin() >= 4) {
            for (auto itr = pool.begin(); itr != last; ++itr) {
                if (itr->wait_for(0ms) != std::future_status::ready) {
                    continue;
                }

                itr->get();
                std::iter_swap(itr, last);
                
                if (itr == --last) {
                    break;
                }
            }
        }
        */

        //if (next == future_list.size()) {
        
        //}
    }

#endif

    return 0;
}

} // namespace httpappserver