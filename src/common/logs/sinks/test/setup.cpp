#include "test/common.hpp"

#include "db/connection.hpp"

#include "logs/structured/channels.hpp"

using namespace sm;
using namespace std::chrono_literals;

#define USE_ORACLE 0

#if USE_ORACLE

static constexpr db::ConnectionConfig kConfig = {
    .port = 1521,
    .host = "localhost",
    .user = "TEST_USER",
    .password = "TEST_USER",
    .database = "FREEPDB1",
    .timeout = 1s
};

static constexpr db::DbType kType = db::DbType::eOracleDB;

#else

static constexpr db::ConnectionConfig kConfig = {
    .host = "testdb.db"
};

static constexpr db::DbType kType = db::DbType::eSqlite3;

#endif

LOG_MESSAGE_CATEGORY(TestLog, "Tests");

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(kType, { .logQueries=true });
    try {
        logs::structured::setup(env.connect(kConfig));
    } catch (db::DbException& e) {
        for (const auto& frame : e.stacktrace()) {
            fmt::println(stderr, "[{}:{}] {}", frame.source_file(), frame.source_line(), frame.description());
        }
        FAIL(e.what());
    }

    GIVEN("a successfully setup logging environment") {
        THEN("a message can be logged") {
            LOG_INFO(TestLog, "Logging setup complete");

            LOG_INFO(TestLog, "Log message {0}", 5);

            LOG_INFO(TestLog, "Logging with {multiple} {parameters}", fmt::arg("multiple", 1), fmt::arg("parameters", false));

            SUCCEED("No exceptions thrown");
        }
    }

    logs::structured::shutdown();
}
