#include "test/common.hpp"

#include "orm/connection.hpp"

#include "logs/structured/message.hpp"

using namespace sm;

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(db::DbType::eSqlite3).value();
    auto conn = env.connect({.host = "testlogging.db"}).value();

    try {
        auto err = logs::structured::setup(conn);
        REQUIRE(err.isSuccess());


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
