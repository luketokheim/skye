#include <httpmicroservice.hpp>

#include <httpmicroservice/service.hpp>

#include <charconv>
#include <exception>

namespace httpmicroservice {

response make_response(request req)
{
    return response(http::status::ok, req.version());
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
    constexpr auto kMinPort = 1024;
    constexpr auto kMaxPort = 65535;

    // Cloud Run sets the PORT environment variable
    // https://cloud.google.com/run/docs/container-contract#port
    char* env = std::getenv("PORT");
    if (env == nullptr) {
        return kDefaultPort;
    }

    int port = kDefaultPort;

    if (std::from_chars(env, env + std::strlen(env), port).ec != std::errc{}) {
        throw std::invalid_argument("PORT is not a number");
    }

    if ((port < kMinPort) || (port > kMaxPort)) {
        throw std::invalid_argument("PORT is not between 1024 and 65535");
    }

    return port;
}

} // namespace httpmicroservice