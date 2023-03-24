#include <boost/asio/signal_set.hpp>
#include <boost/asio/thread_pool.hpp>
#include <fmt/core.h>
#include <skye/service.hpp>
#include <sqlite3.h>

#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <string>

namespace asio = boost::asio;
namespace http = boost::beast::http;

namespace database {

// Object we will read from the database.
struct Model {
    int id{};
    int randomNumber{};
};

// Custom deleter for unique_ptr/shared_ptr to sqlite3 structs.
struct SQLiteDeleter {
    void operator()(sqlite3* ptr) const;
    void operator()(sqlite3_stmt* ptr) const;
};

/** Connection to the database. Use the SQLite C API. */
class SQLiteContext {
public:
    // RAII, requires that the database is open and the statement is prepared.
    explicit SQLiteContext(const std::string& filename);

    // Model not set if:
    // - No matching row in database
    // - Column types do not match object
    // - Error in executing statement
    std::optional<Model> getRandomModel();

private:
    using UniqueConnection = std::unique_ptr<sqlite3, SQLiteDeleter>;
    using UniqueStatement = std::unique_ptr<sqlite3_stmt, SQLiteDeleter>;

    static UniqueConnection MakeConnection(const std::string& filename);
    static UniqueStatement MakeStatement(sqlite3* db);

    // Prepared statement depends on the connection.
    UniqueConnection connection_;
    UniqueStatement statement_;
};

} // namespace database

// Use fmt to convert database::Model to a JSON string.
template <>
struct fmt::formatter<database::Model> {
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const database::Model& model, FormatContext& ctx) const
    {
        // The format string "replacement fields" use {}. Escape those
        // characters with {{ and }}.
        return fmt::format_to(
            ctx.out(), "{{\"id\":{},\"randomNumber\":{}}}", model.id,
            model.randomNumber);
    }
};

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

SQLiteContext::SQLiteContext(const std::string& filename)
    : connection_{MakeConnection(filename)}, statement_{MakeStatement(
                                                 connection_.get())}
{
}

std::optional<Model> SQLiteContext::getRandomModel()
{
    constexpr int kNumColumn = 2;

    std::optional<Model> model;

    auto* stmt = statement_.get();

    if ((sqlite3_step(stmt) == SQLITE_ROW) &&
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
    constexpr std::string_view kSql{
        "SELECT * FROM world ORDER BY RANDOM() LIMIT 1;"};

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