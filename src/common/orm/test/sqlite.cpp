#include "orm_test_common.hpp"

#include <filesystem>

static constexpr ConnectionConfig kConfig = {
    .host = "test.db"
};

TEST_CASE("sqlite updates") {
    REQUIRE(Environment::isSupported(DbType::eSqlite3));

    auto env = getValue(Environment::create(DbType::eSqlite3));

    if (std::filesystem::is_regular_file("test.db")) {
        std::error_code ec;
        bool ok = std::filesystem::remove("test.db", ec);
        CTASSERTF(ec.value() == 0, "Failed to remove test.db: %s (%s)", ec.message().data(), ec.category().name());
        CTASSERT(ok);
    }

    Connection conn = getValue(env.connect(kConfig));

    SECTION("updates and rollback") {
        getValue(conn.update("CREATE TABLE test (id INTEGER, name VARCHAR2(100))"));

        getValue(conn.update("INSERT INTO test (id, name) VALUES (1, 'test')"));

        Transaction tx(&conn);

        getValue(conn.update("INSERT INTO test (id, name) VALUES (2, 'test')"));
        getValue(conn.update("INSERT INTO test (id, name) VALUES (3, 'test')"));

        checkError(tx.rollback());

        auto results = getValue(conn.select("SELECT * FROM test where id = 2"));
        REQUIRE(results.next().isDone());
    }

    SECTION("selects") {
        getValue(conn.update("CREATE TABLE test (id INTEGER, name VARCHAR2(100))"));

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
            int64 id = results.getInt(0);
            std::string_view name = results.getString(1);

            REQUIRE(id == count);
            REQUIRE(name == fmt::format("test{}", count));

            count += 1;
        }

        REQUIRE(count == 4);
    }
}
