#include "test/common.hpp"

#include "db/connection.hpp"

#include "logs/structured/logging.hpp"

using namespace sm;
using namespace std::chrono_literals;

LOG_MESSAGE_CATEGORY(TestLog, "Tests");

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(db::DbType::eSqlite3, { .logQueries=true });
    auto conn = env.connect({.host="testlogs.db"});
    logs::structured::setup(conn);

    GIVEN("a successfully setup logging environment") {
        THEN("a message can be logged") {
            LOG_INFO(TestLog, "Logging setup complete");

            LOG_INFO(TestLog, "Log message {0}", 5);

            LOG_INFO(TestLog, "Logging with {multiple} {parameters}", fmt::arg("multiple", 1), fmt::arg("parameters", false));

            CHECK(logs::structured::isRunning());
        }
    }

    logs::structured::cleanup();
}
