#include <httpmicroservice.hpp>

#include <httpmicroservice/service.hpp>

#include <boost/lexical_cast.hpp>

#include <exception>
#include <iostream>

namespace httpmicroservice {

response make_response(request req)
{
    return response(http::status::ok, req.version());
}

std::ostream &operator<<(std::ostream &os, const session_stats &stats)
{
    os << "{\"fd\": " << stats.fd << "\", num_request\": " << stats.num_request
       << ", \"bytes_read\": " << stats.bytes_read
       << ", \"bytes_write\": " << stats.bytes_write << ", \"duration\": "
       << std::chrono::duration<double>(stats.duration).count() << "}";
    return os;
}

std::string to_string(const session_stats &stats)
{
    return boost::lexical_cast<std::string>(stats);
}

int run(int port, request_handler handler)
{
    asio::io_context ctx;

    async_run(
        ctx.get_executor(), port,
        [handler](request req) -> asio::awaitable<response> {
            co_return handler(std::move(req));
        });

    ctx.run();

    return 0;
}

int getenv_port()
{
    constexpr auto kDefaultPort = 8080;
    constexpr auto kMinPort = (1 << 10);
    constexpr auto kMaxPort = (1 << 16) - 1;

    // Cloud Run sets the PORT environment variable
    // https://cloud.google.com/run/docs/container-contract#port
    char *env = std::getenv("PORT");
    if (env == nullptr) {
        return kDefaultPort;
    }

    int port = kDefaultPort;
    try {
        port = boost::lexical_cast<int>(env);
    } catch (boost::bad_lexical_cast &) {
        throw std::invalid_argument("PORT is not a number");
    }

    if ((port < kMinPort) || (port > kMaxPort)) {
        throw std::invalid_argument("PORT is not between 1024 and 65535");
    }

    return port;
}

} // namespace httpmicroservice