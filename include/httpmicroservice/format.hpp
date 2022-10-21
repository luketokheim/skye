#include <httpmicroservice/types.hpp>

#include <fmt/core.h>

/**
  Specialize the formatter struct to make our user defined session_stats type
  usable in fmt::print etc. Write out a JSON formatted string.

  https://fmt.dev/latest/api.html#formatting-user-defined-types

  Example usage:

  // Print to stdout
  session_stats stats;
  fmt::print("{}\n", stats);

  // Capture a string representation
  std::string str = fmt::format("{}", stats);
 */
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
