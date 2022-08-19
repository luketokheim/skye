#include <httpmicroservice/service.hpp>
#include <httpmicroservice.hpp>

#include <boost/asio/thread_pool.hpp>

#include <exception>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;
namespace http = httpmicroservice::http;

asio::awaitable<usrv::response> handler(usrv::request req)
{
    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"world\"}";

    co_return res;
}

int main()
{
    try {
        auto port = usrv::getenv_port();

        auto ioc = asio::io_context{};

        asio::thread_pool pool(1);
        auto ex = pool.get_executor();

        usrv::async_run(
            ioc.get_executor(), port, usrv::make_co_handler(ex, handler));

        ioc.run();

        return 0;
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    } catch (...) {
        std::cerr << "unknown exception\n";
    }

    return -1;
}