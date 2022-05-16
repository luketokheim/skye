#pragma once

#include <string_view>

namespace httpappserver {

int run(std::string_view host, std::string_view service);

} // namespace httpappserver
