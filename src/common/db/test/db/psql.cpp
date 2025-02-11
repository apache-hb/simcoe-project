#include "db_test_common.hpp"

using namespace std::chrono_literals;

TEST_CASE("sqlite updates") {
    if (!Environment::isSupported(DbType::ePostgreSQL)) {
        SKIP("PostgreSQL not supported");
    }

    auto env = Environment::create(DbType::ePostgreSQL);

    auto connResult = env.tryConnect(kPostgresConfig);
    if (!connResult.has_value()) {
        SKIP("Failed to connect to database " << connResult.error().message());
    }

    auto conn = std::move(connResult.value());

    if (conn.tableExists("test"))
        checkError(conn.tryUpdateSql("DROP TABLE test"));

    REQUIRE(!conn.tableExists("test"));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));

    REQUIRE(conn.tableExists("test"));

    SECTION("updates and rollback") {
        checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));

        Transaction tx(&conn);

        checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (2, 'test')"));
        checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (3, 'test')"));

        tx.rollback();

        auto results = getValue(conn.trySelectSql("SELECT * FROM test where id = 2"));
        REQUIRE(results.next().isDone());
    }

    SECTION("selects") {
        checkError(conn.tryUpdateSql(R"(
            INSERT INTO test
                (id, name)
            VALUES
                (1, 'test1'),
                (2, 'test2'),
                (3, 'test3')
        )"));

        ResultSet results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));

        int count = 1;
        while (results.next().isSuccess()) {
            int64_t id = getValue(results.getInt(0));
            std::string_view name = getValue(results.getString(1));

            REQUIRE(id == count);
            REQUIRE(name == fmt::format("test{}", count));

            count += 1;
        }

        REQUIRE(count == 4);
    }
}
