#pragma once

/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <chrono>
#include <iosfwd>
#include <string>

namespace httpmicroservice {

namespace http = boost::beast::http;

using request = http::request<http::string_body>;
using response = http::response<http::string_body>;
using request_handler = std::function<response(request)>;

struct session_stats {
    int fd = 0;
    int num_request = 0;
    int bytes_read = 0;
    int bytes_write = 0;
    std::chrono::steady_clock::duration duration;
};

std::ostream &operator<<(std::ostream &os, const session_stats &stats);

std::string to_string(const session_stats &stats);

} // namespace httpmicroservice
