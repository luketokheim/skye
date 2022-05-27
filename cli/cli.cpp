#include <httpmicroservice/service.hpp>
#include <httpmicroservice.hpp>

#include <boost/asio/thread_pool.hpp>

#include <exception>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;
namespace http = httpmicroservice::http;

asio::awaitable<usrv::response> handler_api(usrv::request req)
{
    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"api\"}";

    std::this_thread::sleep_for(std::chrono::seconds(1));

    co_return res;
}

asio::awaitable<usrv::response> handler(usrv::request req)
{
    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "application/json");
    res.body() = "{\"hello\": \"world\"}";

    co_return res;
}

struct functor {
    using executor_type = asio::thread_pool::executor_type;

    explicit functor(executor_type ex) : ex_(ex)
    {
        std::cout << "functor {\n";
    }

    ~functor()
    {
        std::cout << "} ~functor\n";
    }

    functor(const functor &other) : ex_(other.ex_)
    {
        std::cout << "functor(other) {\n";
    }

    functor &operator=(const functor &other)
    {
        std::cout << "functor = other {\n";
        ex_ = other.ex_;
        return *this;
    }

    asio::awaitable<usrv::response> operator()(usrv::request req)
    {
        if (req.target().starts_with("/api")) {
            auto res = co_await co_spawn(
                ex_, handler_api(std::move(req)), asio::use_awaitable);

            co_return res;
        } else {
            auto res = co_await handler(std::move(req));

            co_return res;
        }
    }

    executor_type ex_;
};

int main()
{
    try {
        auto port = usrv::getenv_port();

        asio::thread_pool pool(1);

        asio::io_context ctx;
        usrv::async_run(
            ctx.get_executor(), port,
            usrv::make_handler(pool.get_executor(), handler));

        ctx.run();

        return 0;
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    } catch (...) {
        std::cerr << "unknown exception\n";
    }

    return -1;
}