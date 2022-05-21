#include <httpmicroservice/session.hpp>

#include "mock_sock.hpp"

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>

#include <future>
#include <vector>

namespace asio = boost::asio;
namespace http = httpmicroservice::http;
namespace usrv = httpmicroservice;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    using buffer = std::vector<uint8_t>;

    asio::io_context ctx;
    test::mock_sock<buffer> s(ctx.get_executor());

    s.set_rx(buffer(Data, Data + Size));

    auto handler = [](usrv::request req) {
        usrv::response res(http::status::ok, req.version());
        return res;
    };

    auto future = co_spawn(
        ctx,
        usrv::session(s, handler, std::make_optional<usrv::session_stats>()),
        asio::use_future);

    ctx.run();

    if (future.valid()) {
        auto stats = future.get();
    }

    return 0;
}