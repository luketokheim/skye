#pragma once

#include <functional>
#include <string_view>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

namespace httpappserver {

namespace http = boost::beast::http;

using request_type = http::request<http::string_body>;
using response_type = http::response<http::string_body>;
using handler_type = std::function<response_type(request_type)>;

response_type make_response(request_type req);

int run(std::string_view host, std::string_view service, handler_type handler);

} // namespace httpappserver
