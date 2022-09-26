/// Set _WIN32_WINNT to the default for current Windows SDK
#if defined(_WIN32) && !defined(_WIN32_WINNT)
#include <SDKDDKVer.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <fmt/core.h>

#include <exception>
#include <memory>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

const auto kRequestSizeLimit = 1024 * 32;

class session : public std::enable_shared_from_this<session> {
public:
    session(tcp::socket socket)
        : stream_(std::move(socket)), buffer_{kRequestSizeLimit}, req_{}
    {
    }

    void start()
    {
        do_read();
    }

private:
    void do_read()
    {
        req_ = {};

        auto self = shared_from_this();
        http::async_read(
            stream_, buffer_, req_,
            [this, self](auto ec, auto bytes_transferred) {
                if (ec == http::error::end_of_stream) {
                    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
                    return;
                }

                if (ec) {
                    return;
                }

                // Request handler
                res_ = {http::status::ok, req_.version()};
                res_.keep_alive(req_.keep_alive());
                res_.set(http::field::content_type, "application/json");
                res_.body() = "{\"hello\": \"world\"}";
                res_.prepare_payload();

                http::async_write(
                    stream_, res_,
                    [this, self](auto ec, auto bytes_transferred) {
                        if (ec) {
                            return;
                        }

                        if (res_.need_eof()) {
                            stream_.socket().shutdown(
                                tcp::socket::shutdown_send, ec);
                            return;
                        }

                        do_read();
                    });
            });
    }

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
};

class server {
public:
    explicit server(asio::io_context& ioc) : acceptor_(ioc, {tcp::v4(), 8080})
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept([this](auto ec, auto socket) {
            if (!ec) {
                // Launch session
                std::make_shared<session>(std::move(socket))->start();
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
};

int main()
{
    try {
        asio::io_context ioc;

        server s(ioc);

        ioc.run();

        return 0;
    } catch (std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        fmt::print(stderr, "unknown exception\n");
    }

    return -1;
}