#include "db_test_common.hpp"

#include "core/string.hpp"

void checkError(const DbError& err) {
    if (!err.isSuccess()) {
        for (const auto& frame : err.stacktrace()) {
            fmt::println(stderr, "[{}:{}] {}", frame.source_file(), frame.source_line(), frame.description());
        }
        FAIL(err.message() << " (" << err.code() << ")");
    }
}

ConnectionConfig makeSqliteTestDb(const std::string& name, const ConnectionConfig& extra) {
    std::filesystem::path root = "test-data";
    std::filesystem::path path = root / (name + ".db");
    // create all directories up to the file
    std::filesystem::create_directories(path.parent_path());

    ConnectionConfig result = extra;
    result.host = path.string();
    return result;
}

void checkOracleTestUser(db::Environment& env) {
    auto maybeDb = env.tryConnect(kOracleSYSDBA);
    if (!maybeDb.has_value()) {
        return;
    }

    auto conn = std::move(maybeDb.value());

    if (!conn.userExists("TEST_USER")) {
        FAIL("User TEST_USER does not exist, please run `create-oracle-test-user`");
    }
}

std::vector<std::string> loadSqlStatements(const fs::path& path) {
    std::ifstream file{path};
    std::string content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // normalize crlf to lf
    content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());

    // split by semicolon
    // TODO: this doesnt handle strings containing semicolons
    std::vector<std::string> statements = sm::splitString(content, ';');
    for (std::string& stmt : statements) {
        sm::trimWhitespace(stmt);
    }

    // erase all statements starting with -- or empty
    statements.erase(std::remove_if(statements.begin(), statements.end(), [](const std::string& stmt) {
        return stmt.empty() || stmt.starts_with("--");
    }), statements.end());

    return statements;
}
