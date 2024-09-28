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

    auto env = Environment::create(DbType::eOracleDB);

    auto connResult = env.tryConnect(kConfig);
    if (!connResult.has_value()) {
        SKIP("Failed to connect to database " << connResult.error().message());
    }

    auto conn = std::move(connResult.value());

#if 0
    auto allTables = getValue(conn.trySelectSql("SELECT table_name FROM user_tables"));
    for (auto &row : allTables) {
        for (int i = 0; i < row.getColumnCount(); i++) {
            auto info = row.getColumnInfo(i).value_or(ColumnInfo{});
            fmt::println(stderr, "Column {}: {} = `{}`", i, info.name, row.get<std::string>(i).value_or("not a string"));
        }
    }
#endif

    if (getValue(conn.tableExists("test")))
        getValue(conn.tryUpdateSql("DROP TABLE test"));

    getValue(conn.tryUpdateSql("CREATE TABLE test (id NUMBER, name CHARACTER VARYING(100))"));

    REQUIRE(conn.tableExists("test").value_or(false));


    GIVEN("a connection") {
        THEN("simple sql operations work") {
            getValue(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));

            checkError(conn.commit());

            Transaction tx(&conn);

            getValue(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (2, 'test')"));

            tx.rollback();

            auto results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));
            int count = 0;

            while (results.next().isSuccess()) {
                int64 id = getValue(results.getInt(0));
                std::string_view name = getValue(results.getString(1));

                REQUIRE(id == 1);
                REQUIRE(name == "test");

                count++;
            }

            REQUIRE(count == 1);
        }

        THEN("it has a version") {
            getValue(conn.clientVersion());
            getValue(conn.serverVersion());
        }

        THEN("binding variables") {
            auto stmt = getValue(conn.prepareUpdate("INSERT INTO test (id, name) VALUES (:id, :name)"));
            stmt.bind("id").toInt(1);
            stmt.bind("name").toString("test");

            getValue(stmt.update());

            auto results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));
            int count = 0;

            while (!results.isDone()) {
                int64 id = getValue(results.getInt(0));
                std::string_view name = getValue(results.getString(1));

                REQUIRE(id == 1);
                REQUIRE(name == "test");

                count++;

                (void)(results.next());
            }

            REQUIRE(count == 1);
        }
    }

    SECTION("Blob IO") {
        if (conn.tableExists("blob_test").value_or(false))
            getValue(conn.tryUpdateSql("DROP TABLE blob_test"));

        getValue(conn.tryUpdateSql("CREATE TABLE blob_test (id INTEGER, data BLOB)"));

        Blob blob{100};
        for (size_t i = 0; i < blob.size(); i++) {
            blob[i] = static_cast<std::byte>(i);
        }

        auto stmt = getValue(conn.prepareUpdate("INSERT INTO blob_test (id, data) VALUES (1, :blob)"));
        stmt.bind("blob").to(blob);
        getValue(stmt.update());

        ResultSet results = getValue(conn.trySelectSql("SELECT * FROM blob_test"));

        Blob readBlob = getValue(results.getBlob(1));
        REQUIRE(readBlob.size() == blob.size());

        for (size_t i = 0; i < blob.size(); i++) {
            REQUIRE(readBlob[i] == blob[i]);
        }

        REQUIRE(results.next().isDone());
    }
}
