#include "test/common.hpp"

#include "db/connection.hpp"

#include "logs/structured/message.hpp"

using namespace sm;
using namespace std::chrono_literals;

// db::ConnectionConfig cfg {
//     .port = 1521,
//     .host = "localhost",
//     .user = "TEST_USER",
//     .password = "TEST_USER",
//     .database = "FREEPDB1",
//     .timeout = 1s
// };

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(db::DbType::eSqlite3, { .logQueries=true });
    auto conn = env.connect({.host="testlogs.db"});

    if (auto err = logs::structured::setup(conn)) {
        for (const auto& frame : err.stacktrace()) {
            fmt::println(stderr, "  at {0} in {1}:{2}", frame.description(), frame.source_file(), frame.source_line());
        }

        err.raise();
    }

    GIVEN("a successfully setup logging environment") {
        THEN("a message can be logged") {
            LOG_INFO("Logging setup complete");

            LOG_INFO("Log message {index}", 5);

            LOG_INFO("Logging with {multiple} {parameters}", "multiple", true);

            CHECK(logs::structured::isRunning());
        }
    }

    logs::structured::cleanup();
}
