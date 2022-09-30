#pragma once

/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <fmt/core.h>

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

} // namespace httpmicroservice

template <>
struct fmt::formatter<httpmicroservice::session_stats> {
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(
        const httpmicroservice::session_stats& stats, FormatContext& ctx) const
    {
        return fmt::format_to(
            ctx.out(),
            "{{\"fd\": {}, \"num_request\": {}, \"bytes_read\": {}, "
            "\"bytes_write\": {}, \"duration\": {}}}",
            stats.fd, stats.num_request, stats.bytes_read, stats.bytes_write,
            std::chrono::duration<double>(stats.end_time - stats.start_time)
                .count());
    }
};
