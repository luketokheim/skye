#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <httpmicroservice.hpp>

namespace usrv = httpmicroservice;

TEST_CASE("make_response", "[usrv]")
{
    usrv::request req;
    auto res = usrv::make_response(req);

    REQUIRE(res.result_int() == 200);
    REQUIRE(req.version() == res.version());
}
