#include <httpmicroservice.hpp>

#include <httpmicroservice/service.hpp>

#include <charconv>
#include <exception>
#include <string_view>

namespace httpmicroservice {

response make_response(const request& req)
{
    return response{http::status::ok, req.version()};
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

    {
        const std::string_view str{env};
        if (std::from_chars(str.data(), str.data() + str.size(), port).ec !=
            std::errc{}) {
            throw std::invalid_argument("PORT is not a number");
        }
    }

    if ((port < kMinPort) || (port > kMaxPort)) {
        throw std::invalid_argument("PORT is not between 1024 and 65535");
    }

    return port;
}

} // namespace httpmicroservice