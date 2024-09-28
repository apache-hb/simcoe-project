#include "orm_test_common.hpp"
#include <filesystem>

static constexpr ConnectionConfig kConfig = {
    .host = "test.db"
};

TEST_CASE("sqlite updates") {
    REQUIRE(Environment::isSupported(DbType::eSqlite3));

    auto env = Environment::create(DbType::eSqlite3);

    auto conn = env.connect(kConfig);

    if (conn.tableExists("test").value_or(false))
        getValue(conn.tryUpdateSql("DROP TABLE test"));

    REQUIRE(conn.tableExists("test") == DbResult<bool>(false));

    getValue(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));

    REQUIRE(conn.tableExists("test") == DbResult<bool>(true));

    SECTION("updates and rollback") {
        getValue(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));

        Transaction tx(&conn);

        getValue(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (2, 'test')"));
        getValue(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (3, 'test')"));

        tx.rollback();

        auto results = getValue(conn.trySelectSql("SELECT * FROM test where id = 2"));
        REQUIRE(results.next().isDone());
    }

    SECTION("selects") {
        getValue(conn.tryUpdateSql(R"(
            INSERT INTO test
                (id, name)
            VALUES
                (1, 'test1'),
                (2, 'test2'),
                (3, 'test3')
        )"));

        ResultSet results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));

        int count = 1;
        do {
            int64 id = getValue(results.getInt(0));
            std::string_view name = getValue(results.getString(1));

            REQUIRE(id == count);
            REQUIRE(name == fmt::format("test{}", count));

            count += 1;
        } while (!results.next().isDone());

        REQUIRE(count == 4);
    }

    SECTION("ranged for loop selects") {
        getValue(conn.tryUpdateSql(R"(
            INSERT INTO test
                (id, name)
            VALUES
                (1, 'test1'),
                (2, 'test2'),
                (3, 'test3')
        )"));

        ResultSet results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));

        int count = 1;
        for (auto row : results) {
            int64 id = getValue(row.getInt(0));
            std::string_view name = getValue(row.getString(1));

            REQUIRE(id == count);
            REQUIRE(name == fmt::format("test{}", count));

            count += 1;
        }

        REQUIRE(count == 4);
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

TEST_CASE("sqlite connection creation") {
    auto env = Environment::create(DbType::eSqlite3);

    static constexpr JournalMode kJournalModes[] = {
        JournalMode::eDelete,
        JournalMode::eTruncate,
        JournalMode::ePersist,
        JournalMode::eMemory,
        JournalMode::eWal,
        JournalMode::eOff
    };

    static constexpr Synchronous kSynchronousModes[] = {
        Synchronous::eExtra,
        Synchronous::eFull,
        Synchronous::eNormal,
        Synchronous::eOff
    };

    static constexpr LockingMode kLockingModes[] = {
        LockingMode::eRelaxed,
        LockingMode::eExclusive
    };

    std::filesystem::create_directory("sqlite_test");

    // ensure all permutations of journal, synchronous, and locking modes can be connected to
    for (JournalMode journalMode : kJournalModes) {
        for (Synchronous synchronous : kSynchronousModes) {
            for (LockingMode lockingMode : kLockingModes) {
                std::string name = fmt::format("sqlite_test/test-{}-{}-{}.db", toString(journalMode), toString(synchronous), toString(lockingMode));
                ConnectionConfig config = {
                    .host = name,
                    .journalMode = journalMode,
                    .synchronous = synchronous,
                    .lockingMode = lockingMode
                };

                getValue(env.tryConnect(config), fmt::format("{}, {}, {}", toString(journalMode), toString(synchronous), toString(lockingMode)));
            }
        }
    }
}
