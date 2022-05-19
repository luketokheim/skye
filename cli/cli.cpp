#include <httpmicroservice.hpp>

#include <boost/lexical_cast.hpp>

#include <exception>
#include <iostream>

namespace http = httpmicroservice::http;

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

int main()
{
    try {
        auto port = getenv_port();

        return httpmicroservice::run(port, [](auto req) {
            http::response<http::string_body> res(
                http::status::ok, req.version());
            res.set(http::field::content_type, "application/json");
            res.body() = "{\"hello\": \"world\"}";

            return res;
        });
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    } catch (...) {
        std::cerr << "unknown exception\n";
    }

    return -1;
}