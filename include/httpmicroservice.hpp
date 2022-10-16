#pragma once

#include <httpmicroservice/types.hpp>

namespace httpmicroservice {

response make_response(const request& req);

int getenv_port();

} // namespace httpmicroservice
