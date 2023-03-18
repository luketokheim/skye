#pragma once

/// Sets _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <chrono>

namespace skye {

/**
  Use Boost.Beast types directly in the public library interface. Allow the
  user to do simple things easily while still offering access to more advanced
  functionality.
 */
namespace http = boost::beast::http;

/**
  The library is used to create web services call functions via HTTP requests.
  The function inputs may arrive as parameters in the URL, as header values, or
  in the body of the request.

  The library treats the request body as binary data. The user must decode or
  interpret the body in their handler function. For example, the request may
  contain an image or JSON buffer but the library does not validate or parse
  the inputs before calling the user handler.

  Use `http::string_body` to make it simple for users to read the entire body as
  a `std::string`.
 */
using request = http::request<http::string_body>;

/**
  Rationale: We intend to run a function and return the results in the body of
  the response. Since the results are all in memory make it simple to write the
  body with a std::string.
 */
using response = http::response<http::string_body>;

/**
  Rationale: We want a simple mechanism to get transmit and receive byte counts
  and other basic metrics for service observability. The logging could be
  printing to the console or publishing an endpoint for Prometheus based
  monitoring.

  The session(...) function takes an optional reporter function object. If stats
  collection is enabled at compile time then the metrics are collected in the
  HTTP session loop. One session_stats object is intended to represent the
  aggregate data from one session loop. The reporter function object is called
  once per session.
 */
struct session_metrics {
    int fd{};
    int num_request{};
    int bytes_read{};
    int bytes_write{};
    std::chrono::steady_clock::time_point start_time{};
    std::chrono::steady_clock::time_point end_time{};
};

} // namespace skye
