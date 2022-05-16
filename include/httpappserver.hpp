#pragma once

/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace httpappserver {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

asio::awaitable<void> session(tcp::socket socket);

asio::awaitable<void> listen(tcp::endpoint endpoint);

int run();

} // namespace httpappserver
