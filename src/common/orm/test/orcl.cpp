#include "orm_test_common.hpp"

static constexpr ConnectionConfig kConfig = {
    .port = 1521,
    .host = "localhost",
    .user = "TEST_USER",
    .password = "TEST_USER",
    .database = "orclpdb"
};

TEST_CASE("updates") {
    REQUIRE(Environment::isSupported(DbType::eOracleDB));

    auto env = getValue(Environment::create(DbType::eOracleDB));

    auto conn = getValue(env.connect(kConfig));
    if (conn.tableExists("test"))
        getValue(conn.update("DROP TABLE test"));

    GIVEN("a connection") {
        getValue(conn.update("CREATE TABLE test (id NUMBER, name VARCHAR2(100))"));

        getValue(conn.update("INSERT INTO test (id, name) VALUES (1, 'test')"));

        Transaction tx(&conn);

        getValue(conn.update("INSERT INTO test (id, name) VALUES (2, 'test')"));

        checkError(tx.rollback());

        auto results = getValue(conn.select("SELECT * FROM test where id = 2"));
        REQUIRE(results.next());
    }
}
