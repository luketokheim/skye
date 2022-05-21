#pragma once

#include <httpmicroservice/types.hpp>

namespace httpmicroservice {

int run(int port, request_handler handler);

int getenv_port();

response make_response(request req);

} // namespace httpmicroservice
