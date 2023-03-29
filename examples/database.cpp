#include "database.hpp"

#include <boost/asio/signal_set.hpp>
#include <boost/asio/thread_pool.hpp>
#include <fmt/core.h>
#include <skye/service.hpp>
#include <skye/utility.hpp>

#include <cstdio>
#include <exception>
#include <memory>

namespace asio = boost::asio;
namespace http = boost::beast::http;

// Function object with shared database context (that is not copyable).
struct Handler {
    std::shared_ptr<database::SQLiteContext> ctx;

    asio::awaitable<skye::response> operator()(skye::request req) const;
};

// Handle GET /db requests
asio::awaitable<skye::response> Handler::operator()(skye::request req) const
{
    constexpr auto kPath = "/db";
    constexpr auto kContentTypeJson = "application/json";

    if (req.method() != http::verb::get) {
        co_return skye::response{
            http::status::method_not_allowed, req.version()};
    }

    if (req.target() != kPath) {
        co_return skye::response{http::status::not_found, req.version()};
    }

    const auto model = ctx->getRandomModel();
    if (!model) {
        co_return skye::response{http::status::not_found, req.version()};
    }

    skye::response res{http::status::ok, req.version()};
    res.set(http::field::content_type, kContentTypeJson);
    res.body() = fmt::format("{}", *model);

    co_return res;
}

int main()
{
    try {
        const Handler handler{
            std::make_shared<database::SQLiteContext>("database.db")};

        const int port = skye::getenv_port();

        // One thread for the HTTP server.
        asio::io_context ioc{1};
        // One thread for the database handler.
        asio::thread_pool pool{1};

        skye::async_run(ioc, port, skye::make_co_handler(pool, handler));

        // SIGTERM is sent by Docker to ask us to stop (politely)
        // SIGINT handles local Ctrl+C in a terminal
        asio::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait([&ioc](auto ec, auto sig) { ioc.stop(); });

        ioc.run();

        return 0;
    } catch (std::exception& e) {
        fmt::print(stderr, "{}\n", e.what());
    } catch (...) {
        fmt::print(stderr, "unknown exception\n");
    }

    return -1;
}

namespace database {

constexpr int kMinId = 1;
constexpr int kMaxId = 10000;

SQLiteContext::SQLiteContext(const std::string& filename)
    : connection_{MakeConnection(filename)}, statement_{MakeStatement(
                                                 connection_.get())},
      engine_{std::random_device{}()}, uniform_dist_{kMinId, kMaxId}
{
}

std::optional<Model> SQLiteContext::getRandomModel()
{
    constexpr int kNumParam = 1;
    constexpr int kNumColumn = 2;

    const int id = uniform_dist_(engine_);

    std::optional<Model> model;

    auto* stmt = statement_.get();

    if ((sqlite3_bind_parameter_count(stmt) == kNumParam) &&
        (sqlite3_bind_int(stmt, kNumParam, id) == SQLITE_OK) &&
        (sqlite3_step(stmt) == SQLITE_ROW) &&
        (sqlite3_column_count(stmt) == kNumColumn) &&
        (sqlite3_column_type(stmt, 0) == SQLITE_INTEGER) &&
        (sqlite3_column_type(stmt, 1) == SQLITE_INTEGER)) {
        model = {sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1)};
    }

    if (sqlite3_reset(stmt) != SQLITE_OK) {
        model.reset();
    }

    return model;
}

void SQLiteDeleter::operator()(sqlite3* ptr) const
{
    sqlite3_close(ptr);
}

void SQLiteDeleter::operator()(sqlite3_stmt* ptr) const
{
    sqlite3_finalize(ptr);
}

SQLiteContext::UniqueConnection
SQLiteContext::MakeConnection(const std::string& filename)
{
    // Change the following defaults:
    // - Do not create the database if it does not exist
    // - Do not use mutex internally, we are single threaded or will
    //   handle our own locking
    constexpr int kFlags = SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX;

    sqlite3* ptr = nullptr;

    const int ec = sqlite3_open_v2(filename.c_str(), &ptr, kFlags, nullptr);

    // From the SQLite docs:
    //
    // If the database is opened (and/or created) successfully, then
    // SQLITE_OK is returned. Otherwise an error code is returned.
    //
    // ...
    //
    // Whether or not an error occurs when it is opened, resources
    // associated with the database connection handle should be released by
    // passing it to sqlite3_close() when it is no longer required.
    //
    // https://www.sqlite.org/capi3ref.html#sqlite3_open
    UniqueConnection instance{ptr};

    if (ec != SQLITE_OK) {
        throw std::logic_error{sqlite3_errstr(ec)};
    }

    return instance;
}

SQLiteContext::UniqueStatement SQLiteContext::MakeStatement(sqlite3* db)
{
    constexpr std::string_view kSql{"SELECT * FROM world WHERE id=?;"};

    sqlite3_stmt* ptr = nullptr;

    const int ec =
        sqlite3_prepare_v2(db, kSql.data(), kSql.size(), &ptr, nullptr);

    UniqueStatement instance{ptr};

    if (ec != SQLITE_OK) {
        throw std::logic_error{sqlite3_errstr(ec)};
    }

    return instance;
}

} // namespace database