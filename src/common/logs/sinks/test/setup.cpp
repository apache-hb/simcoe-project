#include "test/common.hpp"

#include "db/connection.hpp"

#include "logs/structured/channels.hpp"

using namespace sm;
using namespace std::chrono_literals;

static constexpr db::ConnectionConfig kOracleConfig = {
    .port = 1521,
    .host = "localhost",
    .user = "TEST_USER",
    .password = "TEST_USER",
    .database = "FREEPDB1",
    .timeout = 1s
};

LOG_MESSAGE_CATEGORY(TestLog, "Tests");

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(db::DbType::eSqlite3, { .logQueries=true });
    logs::structured::setup(env.connect({.host="testlogs.db"}));

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
