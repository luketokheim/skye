#include <skye/service.hpp>

#include <charconv>
#include <string_view>

namespace skye {

int getenv_port()
{
    constexpr int kDefaultPort = 8080;
    constexpr int kMinPort = 1024;
    constexpr int kMaxPort = 65535;

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
            return kDefaultPort;
        }
    }

    if ((port < kMinPort) || (port > kMaxPort)) {
        return kDefaultPort;
    }

    return port;
}

} // namespace skye