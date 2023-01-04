#pragma once

/// Sets _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <chrono>

namespace httpmicroservice {

namespace http = boost::beast::http;

/**
  Rationale: We intend to create callable microservices that will receive
  binary, image, or JSON data in the body of the request. Make it simple to
  retrieve the entire body as a std::string.
 */
using request = http::request<http::string_body>;

/**
  Rationale: We intend to run a function and return the results in the body of
  the response. Since the results are all in memory make it simple to set the
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
struct session_stats {
    int fd{0};
    int num_request{0};
    int bytes_read{0};
    int bytes_write{0};
    std::chrono::steady_clock::time_point start_time{};
    std::chrono::steady_clock::time_point end_time{};
};

} // namespace httpmicroservice
