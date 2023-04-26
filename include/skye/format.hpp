//
// skye/format.hpp
//
// Copyright 2023 Luke Tokheim
//
/**
  Use the fmt library to convert structs to JSON strings. Allow the user to
  choose whether to introduce a dependency on fmt.
*/
#ifndef SKYE_FORMAT_HPP_
#define SKYE_FORMAT_HPP_

#include <skye/types.hpp>

#include <fmt/core.h>

#include <chrono>

/**
  Convert SessionMetrics to a JSON string. Specialize the formatter struct so
  SessionMetrics works with fmt::print.

  https://fmt.dev/latest/api.html#formatting-user-defined-types

  Cloud service providers usually support basic structured logging for apps that
  print JSON to stdout.

  Example usage:

  // Print to stdout
  SessionMetrics metrics;
  fmt::print("{}\n", metrics);

  // Or convert to string
  std::string str = fmt::format("{}", metrics);
*/
template <>
struct fmt::formatter<skye::SessionMetrics> {
    constexpr static auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const skye::SessionMetrics& m, FormatContext& ctx) const
    {
        return fmt::format_to(
            ctx.out(),
            "{{\"fd\":{},\"num_request\":{},\"bytes_read\":{},"
            "\"bytes_write\":{},\"duration\":{}}}",
            m.fd, m.num_request, m.bytes_read, m.bytes_write,
            std::chrono::duration<double>(m.end_time - m.start_time).count());
    }
};

#endif // SKYE_FORMAT_HPP_
