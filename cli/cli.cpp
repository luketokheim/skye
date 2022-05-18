#include <httpmicroservice.hpp>

#include <boost/lexical_cast.hpp>

#include <exception>
#include <iostream>

int getenv_port()
{
    constexpr auto kDefaultPort = 8080;
    constexpr auto kMinPort = 1 << 10;
    constexpr auto kMaxPort = 1 << 16;

    // Cloud Run sets the PORT environment variable
    // https://cloud.google.com/run/docs/container-contract#port
    char *env = std::getenv("PORT");
    if (env == nullptr) {
        return kDefaultPort;
    }
    
    try {
        int port = boost::lexical_cast<int>(port);
        if ((port >= kMinPort) && (port <= kMaxPort)) {
            return port;
        }
    } catch (boost::bad_lexical_cast &) {
    }

    return kDefaultPort;
}

int main(int argc, char *argv[])
{
    try {
        using namespace httpmicroservice;

        auto port = getenv_port();

        return run(port, [](auto req) {
            response_type res(http::status::ok, req.version());
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