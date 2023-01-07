#pragma once

#include <httpmicroservice/types.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <fmt/core.h>

#include <chrono>
#include <type_traits>

namespace httpmicroservice {

namespace asio = boost::asio;

// 1 MB request limit
constexpr auto kRequestSizeLimit = 1000 * 1000;

/**
  The HTTP session loop. In this context, a session is multiple HTTP/1.1
  requests with implicit keep alive over one TCP socket stream. The requests
  are serialized one after the other.

  loop {
      request = read(stream)

      response = handler(request)

      write(stream, response)
  }

  If you supply a reporter function object then it is called once after the
  request loop with the aggregate stats.
 */
template <typename AsyncStream, typename Handler, typename Reporter>
asio::awaitable<void>
session(AsyncStream stream, Handler handler, Reporter reporter)
{
    static_assert(
        std::is_invocable_r_v<asio::awaitable<response>, Handler, request>,
        "Handler type requirements not met");

    constexpr bool kEnableStats =
        std::is_invocable_r_v<void, Reporter, const session_stats&>;

    session_stats stats;
    if constexpr (kEnableStats) {
        stats.fd = stream.native_handle();
        stats.start_time = std::chrono::steady_clock::now();
    }

    boost::beast::flat_buffer buffer{kRequestSizeLimit};

    for (;;) {
        // req = read(...)
        request req;
        {
            auto [ec, bytes_read] =
                co_await http::async_read(stream, buffer, req);

            if (ec == http::error::end_of_stream) {
                stream.shutdown(AsyncStream::shutdown_send, ec);
                break;
            }

            if (ec) {
                if (ec != asio::error::connection_reset &&
                    ec != asio::error::connection_aborted) {
                    fmt::print("async_read = {} {}\n", ec.value(), ec.message());
                }
                break;
            }

            if constexpr (kEnableStats) {
                stats.bytes_read += bytes_read;
            }
        }

        const bool keep_alive = req.keep_alive();

        // res = handler(req)
        response res = co_await std::invoke(handler, std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        // write(res)
        auto [ec, bytes_write] = co_await http::async_write(stream, res);

        if (ec) {
            if (ec != asio::error::connection_reset &&
                ec != asio::error::connection_aborted) {
                fmt::print("async_write = {} {}\n", ec.value(), ec.message());
            }
            break;
        }

        if constexpr (kEnableStats) {
            ++stats.num_request;
            stats.bytes_write += bytes_write;
        }

        if (res.need_eof()) {
            stream.shutdown(AsyncStream::shutdown_send, ec);
            break;
        }
    }

    if constexpr (kEnableStats) {
        stats.end_time = std::chrono::steady_clock::now();
        reporter(stats);
    }
}

} // namespace httpmicroservice