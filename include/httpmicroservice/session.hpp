#pragma once

/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <httpmicroservice/types.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <functional>
#include <optional>

namespace httpmicroservice {

namespace asio = boost::asio;

constexpr auto kRequestSizeLimit = 1 << 20;

template <typename AsyncStream, typename Handler>
asio::awaitable<std::optional<session_stats>> session(
    AsyncStream stream, Handler &&handler, std::optional<session_stats> stats)
{
    std::chrono::steady_clock::time_point start_time;
    if (stats) {
        stats->fd = stream.native_handle();
        start_time = std::chrono::steady_clock::now();
    }

    boost::beast::flat_buffer buffer(kRequestSizeLimit);
    boost::system::error_code ec;

    for (;;) {
        // req = read(...)
        request req;
        {
            auto bytes_read = co_await http::async_read(
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

        auto keep_alive = req.keep_alive();

        // res = handler(req)
        response res = co_await std::invoke(
            std::forward<Handler>(handler), std::move(req));
        res.prepare_payload();
        res.keep_alive(keep_alive);

        // write(res)
        auto bytes_write = co_await http::async_write(
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
        stats->duration = std::chrono::steady_clock::now() - start_time;
    }

    co_return stats;
}

} // namespace httpmicroservice