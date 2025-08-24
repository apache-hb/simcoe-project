#include <catch2/benchmark/catch_benchmark.hpp>

#include "test/db_test_common.hpp"

#include "logs/appenders/channels.hpp"
#include "logs/logger.hpp"

#if _WIN32
#   include "core/win32.hpp"
#endif

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
#if _WIN32
    SetProcessAffinityMask(GetCurrentProcess(), 0b1111'1111'1111'1111);
#endif

    logs::create(logs::LoggingConfig { });

    auto env = db::Environment::create(db::DbType::eSqlite3);

    logs::appenders::create(env.connect(makeSqliteTestDb("logs/benchmark")));
    SUCCEED();

    BENCHMARK("Log plain message") {
        LOG_INFO(TestLog, "Benchmark logging message");
    };

    BENCHMARK("Log message with arguments") {
        LOG_INFO(TestLog, "Benchmark logging message with {arg}", fmt::arg("arg", 42));
    };

    logs::appenders::destroy();
}
