#include "test/common.hpp"

#include "db/connection.hpp"

#include "logs/structured/message.hpp"

using namespace sm;
using namespace std::chrono_literals;

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(db::DbType::eSqlite3);
    // db::ConnectionConfig cfg {
    //     .port = 1521,
    //     .host = "localhost",
    //     .user = "TEST_USER",
    //     .password = "TEST_USER",
    //     .database = "FREEPDB1",
    //     .timeout = 1s
    // };

    auto conn = env.connect({.database="testlogs.db"});

    try {
        logs::structured::setup(conn).throwIfFailed();

        // LOG_INFO("Logging setup complete");
    } catch (const db::DbException& err) {
        for (const auto& frame : err.stacktrace()) {
            fmt::println(stderr, "  at {0} in {1}:{2}", frame.description(), frame.source_file(), frame.source_line());
        }

        throw;
    }

    SUCCEED();

    LOG_INFO("Logging setup complete");

    LOG_INFO("Log message {index}", 5);
}
