#include <httpmicroservice.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>

constexpr auto kDefaultHost = "0.0.0.0";
constexpr auto kDefaultPort = "8080";

struct cli_options {
    std::string_view host = kDefaultHost;
    std::string_view port = kDefaultPort;
};

std::optional<cli_options> make_options(int argc, char *argv[])
{
    cli_options options;

    // Cloud Run sets the PORT environment variable 
    // https://cloud.google.com/run/docs/container-contract#port
    {
        char *port = std::getenv("PORT");
        if (port != nullptr) {
            options.port = std::string_view(port);
        }
    }

    for (int i = 1; i < argc; ++i) {
        // Options with a required argument
        if (i + 1 < argc) {
            std::string_view key(argv[i]);
            
            if (key == "--port") {
                options.port = std::string_view(argv[++i]);
                continue;
            }

            if (key == "--host") {
                options.host = std::string_view(argv[++i]);
                continue;
            }
        }

        // Unknown option
        return {};
    }

    return std::make_optional(std::move(options));
}

int main(int argc, char *argv[])
{
    const auto options = make_options(argc, argv);
    if (!options) {
        std::cout << "Usage: cli [OPTION]...\n\n"
                     "Options:\n"
                     "  --host arg (=0.0.0.0)\n"
                     "  --port arg (=8080)\n";
        return -1;
    }

    try {
        namespace http = boost::beast::http;

        return httpmicroservice::run(
            options->host, options->port, [](auto req) {
                httpmicroservice::response_type res{
                    http::status::ok, req.version()};
                res.set(http::field::content_type, "application/json");
                res.body() = "{\"hello\": \"world\"}";

                return res;
            });
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    } catch (...) {
    }

    return -1;
}