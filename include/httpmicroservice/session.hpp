#pragma once

#include <httpmicroservice/types.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <chrono>
#include <optional>

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
 */
template <typename AsyncStream, typename Handler>
asio::awaitable<std::optional<session_stats>>
session(AsyncStream stream, Handler handler, std::optional<session_stats> stats)
{
    if (stats) {
        stats->fd = stream.native_handle();
        stats->start_time = std::chrono::steady_clock::now();
    }

    boost::beast::flat_buffer buffer(kRequestSizeLimit);
    boost::system::error_code ec;

    for (;;) {
        // req = read(...)
        request req;
        {
            const auto bytes_read = co_await http::async_read(
                stream, buffer, req,
                asio::redirect_error(asio::use_awaitable, ec));

            if (ec == http::error::end_of_stream) {
                stream.shutdown(AsyncStream::shutdown_send, ec);
                break;
            }

            if (ec) {
                break;
            }

            if (stats) {
                stats->bytes_read += bytes_read;
            }
        }

        const bool keep_alive = req.keep_alive();

        // res = handler(req)
        response res = co_await std::invoke(handler, std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        // write(res)
        const auto bytes_write = co_await http::async_write(
            stream, res, asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            break;
        }

        if (stats) {
            ++stats->num_request;
            stats->bytes_write += bytes_write;
        }

        if (res.need_eof()) {
            stream.shutdown(AsyncStream::shutdown_send, ec);
            break;
        }
    }

    if (stats) {
        stats->end_time = std::chrono::steady_clock::now();
    }

    co_return stats;
}

} // namespace httpmicroservice