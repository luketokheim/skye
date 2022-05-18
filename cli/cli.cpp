#include <httpmicroservice.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>

struct cli_options {
    std::string_view host = "0.0.0.0";
    std::string_view port = "8080";
};

std::optional<cli_options> make_options(int argc, char **argv)
{
    cli_options options;
    
    if (const char* port = std::getenv("PORT")) {
        options.port = port;
    }

    for (int i = 1; i < argc; ++i) {
        std::string_view name(argv[i]);
        if (name == "--port") {
            if (++i < argc) {
                options.port = argv[i];
                continue;
            }
        } else if (name == "--host") {
            if (++i < argc) {
                options.host = argv[i];
                continue;
            }
        }

        // Unknown option
        return {};
    }

    return std::make_optional(std::move(options));
}

int main(int argc, char **argv)
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
        using namespace httpmicroservice;

        return run(options->host, options->port, [](auto req) {
            response_type res{http::status::ok, req.version()};
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