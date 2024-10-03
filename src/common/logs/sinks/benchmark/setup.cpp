#include <catch2/benchmark/catch_benchmark.hpp>
#include "test/common.hpp"

#include "db/connection.hpp"

#include "logs/structured/channels.hpp"

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

LOG_MESSAGE_CATEGORY(TestLog, "Tests");

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(db::DbType::eSqlite3);

    logs::structured::setup(env.connect({.host="benchlogs.db"}));
    SUCCEED();

    BENCHMARK("Log plain message") {
        LOG_INFO(TestLog, "Benchmark logging message");
    };

    BENCHMARK("Log message with arguments") {
        LOG_INFO(TestLog, "Benchmark logging message with {arg}", fmt::arg("arg", 42));
    };

    logs::structured::shutdown();
}
