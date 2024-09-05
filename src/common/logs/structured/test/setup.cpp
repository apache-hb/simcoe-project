#include "test/common.hpp"

#include "orm/connection.hpp"

#include "logs/structured/message.hpp"

using namespace sm;
using namespace std::chrono_literals;

TEST_CASE("Setup logging") {
    try {
        auto env = db::Environment::create(db::DbType::eSqlite3);
        auto conn = env.connect({.host = "testlogging.db"});

        logs::structured::setup(conn).throwIfFailed();

        // LOG_INFO("Logging setup complete");
    } catch (const db::DbException& err) {
        for (const auto& frame : err.stacktrace()) {
            fmt::println(stderr, "  at {0} in {1}:{2}", frame.description(), frame.source_file(), frame.source_line());
        }
        FAIL(err.what());
    }

    SUCCEED();

    LOG_INFO("Logging setup complete");

    LOG_INFO("Log message {index}", 5);
}
