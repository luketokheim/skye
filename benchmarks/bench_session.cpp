#include "../tests/mock_sock.hpp"
#include "../tests/test.hpp"

#include <benchmark/benchmark.h>
#include <boost/asio.hpp>
#include <skye/session.hpp>

#include <cassert>
#include <exception>

namespace asio = boost::asio;
namespace http = boost::beast::http;

// GET / HTTP/1.1
//
// Reponds with N random characters.
//
void BM_Session_Get(benchmark::State& state)
{
    using buffer = std::string;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_socket = default_token::as_default_on_t<
        test::MockSock<buffer, asio::io_context::executor_type>>;

    constexpr auto kContentType = "text/plain";

    const buffer data = "GET / HTTP/1.1\r\n\r\n";
    const auto body = test::make_random_string<buffer>(
        static_cast<std::size_t>(state.range(0)));

    const auto handler =
        [&body](skye::request req) -> asio::awaitable<skye::response> {
        assert(req.body().empty());

        skye::response res{http::status::ok, req.version()};
        res.set(http::field::content_type, kContentType);
        res.body() = body;

        co_return res;
    };

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};
    s.set_rx(data);

    for (auto _ : state) {
        co_spawn(
            ctx.get_executor(), skye::session(s, handler, false), [](auto ptr) {
                if (ptr) {
                    std::rethrow_exception(ptr);
                }
            });

        const auto count = ctx.run();

        assert(s.get_tx().ends_with(body));

        benchmark::DoNotOptimize(count);
    }
}

BENCHMARK(BM_Session_Get)->Range(1 << 8, 1 << 20);

namespace skye {

template <typename AsyncStream, typename Handler, typename Reporter>
asio::awaitable<void>
session_parser(AsyncStream stream, Handler handler, Reporter reporter)
{
    static_assert(
        std::is_invocable_r_v<asio::awaitable<response>, Handler, request>,
        "Handler type requirements not met");

    constexpr bool kEnableMetrics =
        std::is_invocable_r_v<void, Reporter, const SessionMetrics&>;

    SessionMetrics metrics;
    if constexpr (kEnableMetrics) {
        metrics.fd = stream.native_handle();
        metrics.start_time = std::chrono::steady_clock::now();
    }

    boost::beast::flat_static_buffer<8192> buffer;

    for (;;) {
        http::request_parser<http::string_body> parser;

        // req = read(...)
        {
            auto [ec, bytes_read] =
                co_await http::async_read(stream, buffer, parser);

            if (ec == http::error::end_of_stream) {
                stream.shutdown(AsyncStream::shutdown_send, ec);
                break;
            }

            if (ec) {
                break;
            }

            if constexpr (kEnableMetrics) {
                metrics.bytes_read += bytes_read;
            }
        }

        const bool keep_alive = parser.keep_alive();

        // res = handler(req)
        response res =
            co_await std::invoke(handler, std::move(parser.release()));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        // write(res)
        auto [ec, bytes_write] = co_await http::async_write(stream, res);

        if (ec) {
            break;
        }

        if constexpr (kEnableMetrics) {
            ++metrics.num_request;
            metrics.bytes_write += bytes_write;
        }

        if (res.need_eof()) {
            stream.shutdown(AsyncStream::shutdown_send, ec);
            break;
        }
    }

    if constexpr (kEnableMetrics) {
        metrics.end_time = std::chrono::steady_clock::now();
        std::invoke(reporter, metrics);
    }

    co_return;
}

} // namespace skye

// GET / HTTP/1.1
//
// Reponds with N random characters.
//
void BM_Session_Parser_Get(benchmark::State& state)
{
    using buffer = std::string;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_socket = default_token::as_default_on_t<
        test::MockSock<buffer, asio::io_context::executor_type>>;

    constexpr auto kContentType = "text/plain";

    const buffer data = "GET / HTTP/1.1\r\n\r\n";
    const auto body = test::make_random_string<buffer>(
        static_cast<std::size_t>(state.range(0)));

    const auto handler =
        [&body](skye::request req) -> asio::awaitable<skye::response> {
        assert(req.body().empty());

        skye::response res{http::status::ok, req.version()};
        res.set(http::field::content_type, kContentType);
        res.body() = body;

        co_return res;
    };

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};
    s.set_rx(data);

    for (auto _ : state) {
        co_spawn(
            ctx.get_executor(), skye::session_parser(s, handler, false),
            [](auto ptr) {
                if (ptr) {
                    std::rethrow_exception(ptr);
                }
            });

        const auto count = ctx.run();

        assert(s.get_tx().ends_with(body));

        benchmark::DoNotOptimize(count);
    }
}

BENCHMARK(BM_Session_Parser_Get)->Range(1 << 8, 1 << 20);

// Post / HTTP/1.1
//
// N random characters...
//
// Reponds with the message text
//
// Yes,I got your message!
void BM_Session_Post(benchmark::State& state)
{
    using buffer = std::string;
    using default_token = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using tcp_socket = default_token::as_default_on_t<
        test::MockSock<buffer, asio::io_context::executor_type>>;

    constexpr auto kContentType = "text/plain";
    constexpr auto kBody = "Yes,I got your message!";

    const auto body = test::make_random_string<buffer>(
        static_cast<std::size_t>(state.range(0)));
    const buffer data = "POST / HTTP/1.1\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: " +
                        std::to_string(body.size()) + "\r\n\r\n" + body;

    const auto handler =
        [&body](skye::request req) -> asio::awaitable<skye::response> {
        assert(req.body() == body);

        skye::response res{http::status::ok, req.version()};
        res.set(http::field::content_type, kContentType);
        res.body() = kBody;

        co_return res;
    };

    asio::io_context ctx;
    tcp_socket s{ctx.get_executor()};
    s.set_rx(data);

    for (auto _ : state) {
        co_spawn(
            ctx.get_executor(), skye::session(s, handler, false), [](auto ptr) {
                if (ptr) {
                    std::rethrow_exception(ptr);
                }
            });

        const auto count = ctx.run();

        assert(s.get_tx().ends_with(kBody));

        benchmark::DoNotOptimize(count);
    }
}

BENCHMARK(BM_Session_Post)->Range(1 << 8, 1 << 20);
