#include "orm_test_common.hpp"

static constexpr ConnectionConfig kConfig = {
    .host = "test.db"
};

TEST_CASE("sqlite updates") {
    REQUIRE(Environment::isSupported(DbType::eSqlite3));

    auto env = Environment::create(DbType::eSqlite3);

    auto conn = env.connect(kConfig);

    if (conn.tableExists("test").value_or(false))
        getValue(conn.update("DROP TABLE test"));

    REQUIRE(conn.tableExists("test") == DbResult<bool>(false));

    getValue(conn.update("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));

    REQUIRE(conn.tableExists("test") == DbResult<bool>(true));

    SECTION("updates and rollback") {
        getValue(conn.update("INSERT INTO test (id, name) VALUES (1, 'test')"));

        Transaction tx(&conn);

        getValue(conn.update("INSERT INTO test (id, name) VALUES (2, 'test')"));
        getValue(conn.update("INSERT INTO test (id, name) VALUES (3, 'test')"));

        tx.rollback();

        auto results = getValue(conn.select("SELECT * FROM test where id = 2"));
        REQUIRE(results.next().isDone());
    }

    SECTION("selects") {
        getValue(conn.update(R"(
            INSERT INTO test
                (id, name)
            VALUES
                (1, 'test1'),
                (2, 'test2'),
                (3, 'test3')
        )"));

        ResultSet results = getValue(conn.select("SELECT * FROM test ORDER BY id ASC"));

        int count = 1;
        while (results.next().isSuccess()) {
            int64 id = getValue(results.getInt(0));
            std::string_view name = getValue(results.getString(1));

            REQUIRE(id == count);
            REQUIRE(name == fmt::format("test{}", count));

            count += 1;
        }

        REQUIRE(count == 4);
    }
}
