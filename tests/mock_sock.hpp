#pragma once

#include <boost/asio.hpp>

#include <memory>

namespace test {

/**
  Model the AsyncStream type concept for Boost beast and asio. Allow us to test
  the session(...) loop with random input data and verify the roundtrip.

  https://www.boost.org/doc/libs/1_80_0/doc/html/boost_asio/reference/AsyncReadStream.html
  https://www.boost.org/doc/libs/1_80_0/doc/html/boost_asio/reference/AsyncWriteStream.html
 */
template <typename Buffer>
class mock_sock {
public:
    using executor_type = boost::asio::io_context::executor_type;

    enum shutdown_type { shutdown_receive, shutdown_send, shutdown_both };

    mock_sock(executor_type ex)
        : ex_{ex}, rx_{std::make_shared<Buffer>()},
          tx_{std::make_shared<Buffer>()}
    {
    }

    template <typename MutableBufferSequence, typename ReadToken>
    auto
    async_read_some(const MutableBufferSequence& buffers, ReadToken&& token)
    {
        const auto n =
            boost::asio::buffer_copy(buffers, boost::asio::buffer(*rx_));

        boost::system::error_code ec;
        if (n == 0) {
            ec = boost::asio::error::eof;
        }

        return token(ec, n);
    }

    template <typename ConstBufferSequence, typename WriteToken>
    auto
    async_write_some(const ConstBufferSequence& buffers, WriteToken&& token)
    {
        *tx_ = Buffer(boost::asio::buffer_size(buffers), 0);

        const auto n =
            boost::asio::buffer_copy(boost::asio::buffer(*tx_), buffers);

        boost::system::error_code ec;
        if (n == 0) {
            ec = boost::asio::error::eof;
        }

        return token(ec, n);
    }

    executor_type get_executor()
    {
        return ex_;
    }

    int native_handle() const
    {
        return 1;
    }

    void shutdown(shutdown_type what, boost::system::error_code& ec)
    {
        ec = boost::system::error_code{};
    }

    template <typename SettableSocketOption>
    void set_option(
        const SettableSocketOption& option, boost::system::error_code& ec)
    {
        ec = boost::system::error_code{};
    }

    void set_rx(const Buffer& buf)
    {
        *rx_ = buf;
    }

    Buffer get_tx()
    {
        return *tx_;
    }

private:
    executor_type ex_;
    std::shared_ptr<Buffer> rx_;
    std::shared_ptr<Buffer> tx_;
}; // class mock_sock

} // namespace test