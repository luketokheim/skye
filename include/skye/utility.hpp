//
// skye/utility.hpp
//
// Copyright 2023 Luke Tokheim
//
/**
  Utility functions used in the example apps and to help users make their own
  container based services.
*/
#ifndef SKYE_UTILITY_HPP_
#define SKYE_UTILITY_HPP_

#include <charconv>
#include <cstdlib>
#include <string_view>
#include <system_error>

namespace skye {

/**
  Read the PORT environment variable. Returns 8080 if the PORT variable is not
  set or is not an integer between 1024 and 65535.

  Cloud Run sets the PORT environment variable.
  https://cloud.google.com/run/docs/container-contract#port

  This function clearly does not need to be inlined but make it so since it is
  the only non template function in the whole library.
 */
inline int getenv_port()
{
    constexpr auto kEnvVar = "PORT";
    constexpr int kDefaultPort = 8080;
    constexpr int kMinPort = 1024;
    constexpr int kMaxPort = 65535;

    // Cloud Run sets the PORT environment variable
    // https://cloud.google.com/run/docs/container-contract#port
    char* env = std::getenv(kEnvVar);
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

#endif // SKYE_UTILITY_HPP_
