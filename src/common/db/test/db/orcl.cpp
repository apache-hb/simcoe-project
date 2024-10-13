#include "db_test_common.hpp"

using namespace std::chrono_literals;

TEST_CASE("updates") {
    if (!Environment::isSupported(DbType::eOracleDB)) {
        SKIP("OracleDB not supported");
    }

    auto env = Environment::create(DbType::eOracleDB);

    auto connResult = env.tryConnect(kOracleConfig);
    if (!connResult.has_value()) {
        SKIP("Failed to connect to database " << connResult.error().message());
    }

    auto conn = std::move(connResult.value());

    auto allTables = getValue(conn.trySelectSql("SELECT * FROM user_tables"));
    for (auto &row : allTables) {
        for (int i = 0; i < row.getColumnCount(); i++) {
            auto info = row.getColumnInfo(i).value_or(ColumnInfo{});
            std::string val = row.get<std::string>(i).value_or("not a string");

            (void)info;
            (void)val;
        }
    }

    if (getValue(conn.tableExists("test"))) {
        checkError(conn.tryUpdateSql("DROP TABLE test"));
        REQUIRE(!conn.tableExists("test").value_or(true));
    }

    checkError(conn.tryUpdateSql("CREATE TABLE test (id NUMBER, name VARCHAR2(100))"));

    REQUIRE(conn.tableExists("test").value_or(false));


    GIVEN("a connection") {
        THEN("simple sql operations work") {
            checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'bob')"));

            checkError(conn.commit());

            Transaction tx(&conn);

            checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (2, 'bob')"));

            tx.rollback();

            auto results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));
            int count = 0;

            while (!results.isDone()) {
                int64 id = getValue(results.getInt(0));
                std::string_view name = getValue(results.getString(1));

                REQUIRE(id == 1);
                REQUIRE(name == "bob");

                count++;

                (void)results.next();
            }

            REQUIRE(count == 1);
        }

        THEN("it has a version") {
            getValue(conn.clientVersion());
            getValue(conn.serverVersion());
        }

        THEN("binding variables") {
            conn.setAutoCommit(true);

            auto stmt = getValue(conn.tryPrepareUpdate("INSERT INTO test (id, name) VALUES (:id, :name)"));
            stmt.bind("id").toInt(1);
            stmt.bind("name").toString("test");

            stmt.execute().throwIfFailed();

            int count = 0;

            ResultSet results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));
            for (auto& row : results) {
                int64 id = getValue(row.getInt(0));
                auto name = getValue(row.getString(1));

                REQUIRE(id == 1);
                REQUIRE(name == "test");

                count++;
            }

            REQUIRE(count == 1);
        }
    }

    SECTION("Blob IO") {
        if (conn.tableExists("blob_test").value_or(false))
            checkError(conn.tryUpdateSql("DROP TABLE blob_test"));

        checkError(conn.tryUpdateSql("CREATE TABLE blob_test (id INTEGER, data BLOB)"));

        Blob blob{100};
        for (size_t i = 0; i < blob.size(); i++) {
            blob[i] = static_cast<std::uint8_t>(i);
        }

        auto stmt = getValue(conn.tryPrepareUpdate("INSERT INTO blob_test (id, data) VALUES (1, :blob)"));
        stmt.bind("blob").to(blob);
        checkError(stmt.execute());

        ResultSet results = getValue(conn.trySelectSql("SELECT * FROM blob_test"));

        Blob readBlob = getValue(results.getBlob(1));
        CHECK(readBlob.size() == blob.size());

        for (size_t i = 0; i < blob.size(); i++) {
            CHECK(readBlob[i] == blob[i]);
        }

        CHECK(results.next().isDone());
    }
}
