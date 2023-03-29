#include <fmt/core.h>
#include <sqlite3.h>

#include <memory>
#include <optional>
#include <random>
#include <string>

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

    // Used to randomly select a row by its id field in `getRandomModel`.
    std::mt19937 engine_;
    std::uniform_int_distribution<int> uniform_dist_;
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