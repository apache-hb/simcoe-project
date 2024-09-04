#include "orm_test_common.hpp"

using namespace std::chrono_literals;

static constexpr ConnectionConfig kConfig = {
    .port = 1521,
    .host = "localhost",
    .user = "TEST_USER",
    .password = "TEST_USER",
    .database = "FREEPDB1",
    .timeout = 1s
};

TEST_CASE("updates") {
    if (!Environment::isSupported(DbType::eOracleDB)) {
        SKIP("OracleDB not supported");
    }

    auto env = getValue(Environment::create(DbType::eOracleDB));

    auto connResult = env.connect(kConfig);
    if (!connResult.has_value()) {
        SKIP("Failed to connect to database " << connResult.error().message());
    }

    auto conn = std::move(connResult.value());

    if (conn.tableExists("test"))
        getValue(conn.update("DROP TABLE test"));

    getValue(conn.update("CREATE TABLE test (id NUMBER, name CHARACTER VARYING(100))"));

    GIVEN("a connection") {
        THEN("simple sql operations work") {
            getValue(conn.update("INSERT INTO test (id, name) VALUES (1, 'test')"));

            conn.commit();

            Transaction tx(&conn);

            getValue(conn.update("INSERT INTO test (id, name) VALUES (2, 'test')"));

            tx.rollback();

            auto results = getValue(conn.select("SELECT * FROM test ORDER BY id ASC"));
            int count = 0;

            while (results.next().isSuccess()) {
                int64 id = results.getInt(0);
                std::string_view name = results.getString(1);

                REQUIRE(id == 1);
                REQUIRE(name == "test");

                count++;
            }

            REQUIRE(count == 1);
        }

        THEN("it has a version") {
            getValue(conn.dbVersion());
        }
    }
}