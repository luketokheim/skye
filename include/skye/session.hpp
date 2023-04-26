//
// skye/session.hpp
//
// Copyright 2023 Luke Tokheim
//
/**
  In the skye framework, an HTTP session is multiple requests over one TCP
  socket connection. A session is a coroutine that runs in a keep alive loop.

  Users will not generally directly call the session function but instead use
  run or async_run from the service.hpp header.
*/
#ifndef SKYE_SESSION_HPP_
#define SKYE_SESSION_HPP_

#include <skye/types.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <chrono>
#include <concepts>
#include <functional>
#include <type_traits>

namespace skye {

namespace asio = boost::asio;

// 1 MB request limit
constexpr auto kRequestSizeLimit = 1000 * 1000;

/**
  Inherit requirements from Boost.Beast for a TCP socket stream.
*/
template <typename T>
concept AsyncStream = boost::beast::is_async_stream<T>::value;

/**
  Handler function object must be:
  - CopyConstructible
  - Must be callable with a request and return an awaitable wrapped response
*/
// clang-format off
template <typename T>
concept Handler = std::copy_constructible<T> &&
    std::is_invocable_r_v<asio::awaitable<response>, T, request>;
// clang-format on

/**
  Reporter function object must be:
  - CopyConstructible
  - Must be callable with a SessionMetrics object OR be an integral type

  If Reporter is an integral type disable metrics at compile time.
*/
// clang-format off
template <typename T>
concept Reporter = std::copy_constructible<T> &&
    (std::integral<T> || std::invocable<T, const SessionMetrics&>);
// clang-format on

/**
  The HTTP session loop. In the library, a session is multiple HTTP/1.1 requests
  with implicit keep alive over one TCP socket stream. The requests are
  serialized one after the other.

  loop {
    request = read(stream)

    response = handler(request)

    write(stream, response)
  }

  If the user supplies a reporter function object then that is called once after
  the request loop with the aggregate metrics.

  The session owns the socket stream. The session owns a copy of the handler
  function and a copy of the reporter function.
*/
asio::awaitable<void>
session(AsyncStream auto stream, Handler auto handler, Reporter auto reporter)
{
    constexpr bool kEnableMetrics =
        std::invocable<decltype(reporter), const SessionMetrics&>;

    SessionMetrics metrics;
    if constexpr (kEnableMetrics) {
        metrics.fd = static_cast<int>(stream.native_handle());
        metrics.start_time = std::chrono::steady_clock::now();
    }

    boost::beast::flat_buffer buffer{kRequestSizeLimit};

    for (;;) {
        // req = read(...)
        request req;
        {
            auto [ec, bytes_read] =
                co_await http::async_read(stream, buffer, req);

            if (ec == http::error::end_of_stream) {
                stream.shutdown(decltype(stream)::shutdown_send, ec);
                break;
            }

            if (ec) {
                break;
            }

            if constexpr (kEnableMetrics) {
                metrics.bytes_read += static_cast<int>(bytes_read);
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
            break;
        }

        if constexpr (kEnableMetrics) {
            ++metrics.num_request;
            metrics.bytes_write += static_cast<int>(bytes_write);
        }

        if (res.need_eof()) {
            stream.shutdown(decltype(stream)::shutdown_send, ec);
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

#endif // SKYE_SESSION_HPP_
