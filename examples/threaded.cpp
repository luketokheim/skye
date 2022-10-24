#include <httpmicroservice/format.hpp>
#include <httpmicroservice/service.hpp>

#include <boost/asio/signal_set.hpp>
#include <fmt/core.h>
#include <fmt/std.h>

#include <cstdio>
#include <exception>
#include <thread>

namespace asio = boost::asio;
namespace usrv = httpmicroservice;
namespace http = boost::beast::http;

int get_current_cpu();

bool set_thread_affinity(int i);

asio::awaitable<usrv::response> threaded(usrv::request req)
{
    usrv::response res(http::status::ok, req.version());
    res.set(http::field::content_type, "application/json");
    res.body() = fmt::format(
        "{{\"thread\": \"{}\", \"cpu\": {}}}", std::this_thread::get_id(),
        get_current_cpu());

    co_return res;
}

// Print aggregate session stats to stdout. Called once per socket connection
// which likely includes multiple HTTP requests.
void reporter(const usrv::session_stats& stats)
{
    fmt::print("{}\n", stats);
}

struct thread_list_joiner {
    ~thread_list_joiner()
    {
        for (auto& item : threads) {
            item.join();
        }
    }

    std::vector<std::thread> threads;
};

int main()
{
    try {
        const auto port = usrv::getenv_port();

        auto ioc = asio::io_context{};

        usrv::async_run(ioc, port, threaded);

        // SIGTERM is sent by Docker to ask us to stop (politely)
        // SIGINT handles local Ctrl+C in a terminal
        asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](auto ec, auto sig) { ioc.stop(); });

        const int num_thread =
            static_cast<int>(std::thread::hardware_concurrency());

        // std::vector<std::jthread> thread_list;
        thread_list_joiner list;
        if (num_thread > 1) {
            if (!set_thread_affinity(0)) {
                return -1;
            }

            for (int i = 1; i < num_thread; ++i) {
                list.threads.push_back(std::thread{[&ioc, i]() {
                    if (!set_thread_affinity(i)) {
                        return;
                    }

                    ioc.run();
                }});
            }
        }

        ioc.run();

        return 0;
    } catch (std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        fmt::print(stderr, "unknown exception\n");
    }

    return -1;
}

int get_current_cpu()
{
#if defined(_WIN32)
    return GetCurrentProcessorNumber();
#else
    return sched_getcpu();
#endif
}

bool set_thread_affinity(int i)
{
#if defined(_WIN32)
    if ((i < 0) || (i >= sizeof(DWORD_PTR) * 8)) {
        return false;
    }

    DWORD_PTR mask = static_cast<DWORD_PTR>(1) << i;

    return SetThreadAffinityMask(GetCurrentThread(), mask) != 0;
#else
    if ((i < 0) || (i >= CPU_SETSIZE)) {
        return false;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);

    return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0;
#endif
}