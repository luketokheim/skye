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

using request = http::request<http::string_body>;
using response = http::response<http::string_body>;
using request_handler = std::function<response(request)>;

struct session_stats {
    int fd{0};
    int num_request{0};
    int bytes_read{0};
    int bytes_write{0};
    std::chrono::steady_clock::time_point start_time{};
    std::chrono::steady_clock::time_point end_time{};
};

using session_stats_reporter = std::function<void(const session_stats&)>;

} // namespace httpmicroservice
