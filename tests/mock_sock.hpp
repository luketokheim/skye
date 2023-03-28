#pragma once

#include <boost/asio.hpp>

#include <memory>

namespace test {

/**
  Model the AsyncStream type concept for Boost beast and asio. Allow us to test
  the session(...) loop with random input data and verify the roundtrip.

  https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/AsyncReadStream.html
  https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/AsyncWriteStream.html
 */
template <typename Buffer, typename Executor>
class mock_sock {
public:
    using executor_type = Executor;

    // Need this to specialize default completion handlers for as_tuple_t, etc.
    template <typename Executor1>
    struct rebind_executor {
        typedef mock_sock<Buffer, Executor1> other;
    };

    enum shutdown_type { shutdown_receive, shutdown_send, shutdown_both };

    explicit mock_sock(executor_type ex)
        : ex_{ex}, rx_{std::make_shared<Buffer>()},
          tx_{std::make_shared<Buffer>()}, rx_offset_{}
    {
    }

    template <typename MutableBufferSequence, typename ReadToken>
    auto
    async_read_some(const MutableBufferSequence& buffers, ReadToken&& token)
    {
        if (rx_offset_ >= rx_->size()) {
            return token(boost::asio::error::eof, 0);
        }

        const auto n = boost::asio::buffer_copy(
            buffers,
            boost::asio::buffer(&(*rx_)[rx_offset_], rx_->size() - rx_offset_));

        boost::system::error_code ec;
        if (n == 0) {
            ec = boost::asio::error::eof;
        } else {
            rx_offset_ += n;
        }

        return token(ec, n);
    }

    template <typename ConstBufferSequence, typename WriteToken>
    auto
    async_write_some(const ConstBufferSequence& buffers, WriteToken&& token)
    {
        Buffer buf(boost::asio::buffer_size(buffers), 0);

        const auto n =
            boost::asio::buffer_copy(boost::asio::buffer(buf), buffers);

        boost::system::error_code ec;
        if (n == 0) {
            ec = boost::asio::error::eof;
        } else {
            tx_->insert(tx_->end(), buf.begin(), buf.end());
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
    std::size_t rx_offset_;
}; // class mock_sock

} // namespace test