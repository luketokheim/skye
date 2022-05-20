#include <httpmicroservice/service.hpp>
#include <httpmicroservice.hpp>

#include <exception>
#include <iostream>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;
namespace http = httpmicroservice::http;

usrv::response handler(usrv::request req)
{
    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"world\"}";

    return res;
}

int main()
{
    try {
        auto port = usrv::getenv_port();

        asio::io_context ctx;
        usrv::async_run(ctx.get_executor(), port, handler);

        ctx.run();

        return 0;
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    } catch (...) {
        std::cerr << "unknown exception\n";
    }

    return -1;
}