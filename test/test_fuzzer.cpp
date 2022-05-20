#include <httpmicroservice/session.hpp>

#include <span>

namespace asio = boost::asio;
namespace http = httpmicroservice::http;
namespace usrv = httpmicroservice;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    using buffer = std::span<uint8_t>(Data, Size);

    asio::io_context ctx;
    test::mock_sock<buffer> s(ctx.get_executor());

    const buffer data(Data, Size);
    s.set_rx(data);

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