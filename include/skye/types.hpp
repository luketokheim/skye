//
// skye/types.hpp
//
// Copyright 2023 Luke Tokheim
//
#ifndef SKYE_TYPES_HPP_
#define SKYE_TYPES_HPP_

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <chrono>

namespace skye {

/**
  The framework uses Boost.Beast types directly in its public interface. The
  intent is to allow users to do simple things easily while still offering
  access to more advanced functionality.
 */
namespace http = boost::beast::http;

/**
  The library is used to create web services that call functions via HTTP
  requests. The function inputs may arrive as parameters in the URL, as header
  values, or in the body of the request.

  The library treats the request body as binary data. The user must decode or
  interpret the body in their handler function. For example, the request may
  contain an image or JSON buffer but the library does not validate or parse
  the inputs before calling the user handler.

  Use `http::string_body` to make it simple for users to read the entire body as
  a `std::string`.
 */
using request = http::request<http::string_body>;

/**
  The library calls a function and returns the results as an HTTP response. The
  outputs are the response headers and the body.

  The library treats the response body as binary data. The user must set an
  appropriate content type header.

  For example, to send a JSON buffer set the `http::field::content_type` header
  to ""application/json"".

  Use `http::string_body` to make it simple for users to write the entire body
  as a `std::string`.
 */
using response = http::response<http::string_body>;

/**
  A simple mechanism to record byte counts for incoming and outgoing HTTP
  messages and other basic metrics for service observability. The user supplies
  a reporting callback and can choosem how to log the metrics.

  The library implements a JSON formatter for the metrics object. A quick start
  is to just to call fmt::print("{}\n", metrics) to log to stdout.

  The session(...) function takes an optional reporter function object. If stats
  collection is enabled at compile time then the metrics are collected in the
  HTTP session loop.

  One SessionMetrics object is intended to represent the aggregate data from one
  session loop. The reporter function object is called once per session.
 */
struct SessionMetrics {
    int fd{};
    int num_request{};
    int bytes_read{};
    int bytes_write{};
    std::chrono::steady_clock::time_point start_time{};
    std::chrono::steady_clock::time_point end_time{};
};

} // namespace skye

#endif // SKYE_TYPES_HPP_
