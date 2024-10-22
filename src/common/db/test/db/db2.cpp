#include "db_test_common.hpp"

using namespace std::chrono_literals;

TEST_CASE("updates") {
    if (!Environment::isSupported(DbType::eDB2)) {
        SKIP("OracleDB not supported");
    }

    auto env = Environment::create(DbType::eDB2);

    auto connResult = env.tryConnect(kDB2Config);
    if (!connResult.has_value()) {
        SKIP("Failed to connect to database " << connResult.error().message());
    }

    auto conn = std::move(connResult.value());

    auto clientVersion = conn.clientVersion();
    auto serverVersion = conn.serverVersion();

    CHECK(clientVersion.isKnown());
    CHECK(serverVersion.isKnown());

    if (conn.tableExists("test")) {
        conn.updateSql("DROP TABLE test");
        REQUIRE(!conn.tableExists("test"));
    }

    conn.updateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))");

    REQUIRE(conn.tableExists("test"));
}
