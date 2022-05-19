#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <httpmicroservice.hpp>

TEST_CASE("run")
{
    using namespace httpmicroservice;

    run(8080, [](auto req) {
        return make_response(std::move(req));
    });
}

TEST_CASE("make_request")
{
    using namespace httpmicroservice;

    request_type req;
    auto res = make_response(req);

    REQUIRE(req.version() == res.version());
}
