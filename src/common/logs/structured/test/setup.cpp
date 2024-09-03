#include "test/common.hpp"

#include "orm/connection.hpp"

#include "logs/structured/message.hpp"

using namespace sm;

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(db::DbType::eSqlite3).value();
    auto conn = env.connect({.host = "testlogging.db"}).value();
    if (auto err = logs::structured::setup(conn)) {
        FAIL(err.message());
    }

    SUCCEED();
}
