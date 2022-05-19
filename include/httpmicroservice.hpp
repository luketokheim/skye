#pragma once

/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <functional>
#include <string_view>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

namespace httpmicroservice {

namespace http = boost::beast::http;

using request_type = http::request<http::string_body>;
using response_type = http::response<http::string_body>;
using handler_type = std::function<response_type(request_type)>;

response_type make_response(request_type req);

int run(int port, handler_type handler);

} // namespace httpmicroservice
