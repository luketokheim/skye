#include <httpappserver.hpp>

#include <iostream>
#include <optional>

struct command_line_options {
    std::string_view host = "0.0.0.0";
    std::string_view service = "8080";
};

std::optional<command_line_options> make_options(int argc, char **argv)
{
    command_line_options options;

    for (int i = 1; i < argc; ++i) {
        std::string_view name(argv[i]);
        if (name == "--port") {
            if (++i < argc) {
                options.service = argv[i];
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
                     "Available options:\n"
                     "  --host HOST[=0.0.0.0]\n"
                     "  --port NUMBER[=8080]\n";
        return -1;
    }

    try {
        return httpappserver::run(options->host, options->service);
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    } catch (...) {
    }

    return -1;
}