#pragma once

/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <chrono>
#include <functional>

namespace httpmicroservice {

namespace http = boost::beast::http;

/**
  Rationale: We intend to create callable microservices that will likely receive
  binary or JSON data in the body of the request. Make it simple to retrieve the
  std::string.
 */
using request = http::request<http::string_body>;

/**
  Rationale: We intend to run a C++ function and return the results as JSON
  data in the body. Since the results are all in memory (not in a file) make it
  simple to set with a std::string.
 */
using response = http::response<http::string_body>;

/**
  The session(...) function takes an optional session_stats object. If stats
  collection is enabled at compile time then this object is populated in the
  HTTP session loop. One session_stats object is intended to represent the
  aggregate data from one session(...) loop.
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
