#include <httpmicroservice/types.hpp>

#include <fmt/core.h>

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
