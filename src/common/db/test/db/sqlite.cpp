#include "test/gtest_common.hpp"

#include "db/db.hpp"
#include "db/environment.hpp"
#include "db/transaction.hpp"

using namespace sm;
using namespace sm::db;

#define NEW_TESTDB "testdb" CT_STR(__COUNTER__)

ConnectionConfig makeSqliteTestDb(const std::string& name, const ConnectionConfig& extra = {});

ConnectionConfig makeSqliteTestDb(const std::string& name, const ConnectionConfig& extra) {
    std::filesystem::path root = "test-data";
    std::filesystem::path path = root / (name + ".db");
    // create all directories up to the file
    std::filesystem::create_directories(path.parent_path());

    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }

    ConnectionConfig result = extra;
    result.host = path.string();
    return result;
}

void checkError(const DbError& err) {
    if (!err.isSuccess()) {
        for (const auto& frame : err.stacktrace()) {
            fmt::println(stderr, "[{}:{}] {}", frame.source_file(), frame.source_line(), frame.description());
        }
        GTEST_FAIL() << err.message() << " (" << err.code() << ")";
    }
}

template<typename T>
auto getValue(DbResult<T> result, std::string_view msg = "") {
    if (result.has_value()) {
        GTEST_SUCCEED() << msg;
        return std::move(result.value());
    }

    checkError(result.error());
    throw std::runtime_error("unexpected error");
}

class SqliteTest : public testing::Test {
public:
    std::optional<Environment> env;

    void SetUp() override {
        ASSERT_TRUE(Environment::isSupported(DbType::eSqlite3));
        env = Environment::create(DbType::eSqlite3);
    }
};

TEST_F(SqliteTest, Construction) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    GTEST_SUCCEED() << "Created connection";
}

TEST_F(SqliteTest, CreateTable) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    if (conn.tableExists("test"))
        checkError(conn.tryUpdateSql("DROP TABLE test"));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));

    ASSERT_TRUE(conn.tableExists("test"));
}

TEST_F(SqliteTest, Insert) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));

    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));

    GTEST_SUCCEED() << "Inserted row";
}

TEST_F(SqliteTest, Select) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));

    auto results = getValue(conn.trySelectSql("SELECT * FROM test"));

    ASSERT_TRUE(results.next().isSuccess());
    ASSERT_TRUE(results.isDone());
}

TEST_F(SqliteTest, SelectValues) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));

    auto results = getValue(conn.trySelectSql("SELECT * FROM test"));

    ASSERT_FALSE(results.isDone());

    int64 id = getValue(results.getInt(0));
    std::string_view name = getValue(results.getString(1));

    ASSERT_EQ(id, 1);
    ASSERT_EQ(name, "test");

    ASSERT_TRUE(results.next().isDone());
}

TEST_F(SqliteTest, DoWhileSelect) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test1')"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (2, 'test2')"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (3, 'test3')"));

    auto results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));

    int count = 1;
    do {
        int64 id = getValue(results.getInt(0));
        std::string_view name = getValue(results.getString(1));

        ASSERT_EQ(id, count);
        ASSERT_EQ(name, fmt::format("test{}", count));

        count += 1;
    } while (!results.next().isDone());

    ASSERT_EQ(count, 4);
}

TEST_F(SqliteTest, RangedForSelect) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test1')"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (2, 'test2')"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (3, 'test3')"));

    auto results = getValue(conn.trySelectSql("SELECT * FROM test ORDER BY id ASC"));

    int count = 1;
    for (auto row : results) {
        int64 id = getValue(row.getInt(0));
        std::string_view name = getValue(row.getString(1));

        ASSERT_EQ(id, count);
        ASSERT_EQ(name, fmt::format("test{}", count));

        count += 1;
    }

    ASSERT_EQ(count, 4);
}

TEST_F(SqliteTest, Update) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));

    checkError(conn.tryUpdateSql("UPDATE test SET name = 'updated' WHERE id = 1"));

    auto results = getValue(conn.trySelectSql("SELECT * FROM test"));

    ASSERT_FALSE(results.isDone());

    int64 id = getValue(results.getInt(0));
    std::string_view name = getValue(results.getString(1));

    ASSERT_EQ(id, 1);
    ASSERT_EQ(name, "updated");

    ASSERT_TRUE(results.next().isDone());
}

TEST_F(SqliteTest, Delete) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));
    checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));

    checkError(conn.tryUpdateSql("DELETE FROM test WHERE id = 1"));

    auto results = getValue(conn.trySelectSql("SELECT * FROM test"));

    ASSERT_TRUE(results.next().isDone());
}

TEST_F(SqliteTest, Transactions) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    checkError(conn.tryUpdateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))"));

    {
        db::Transaction tx(&conn);

        checkError(conn.tryUpdateSql("INSERT INTO test (id, name) VALUES (1, 'test')"));
    }

    auto results = getValue(conn.trySelectSql("SELECT * FROM test"));

    ASSERT_TRUE(results.next().isDone());
}

TEST_F(SqliteTest, BlobIO) {
    auto conn = env->connect(makeSqliteTestDb(NEW_TESTDB));

    if (conn.tableExists("blob_test"))
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
    ASSERT_EQ(readBlob.size(), blob.size());

    for (size_t i = 0; i < blob.size(); i++) {
        ASSERT_EQ(readBlob[i], blob[i]);
    }

    ASSERT_TRUE(results.next().isDone());
}

class SqliteCreateTest : public testing::TestWithParam<std::tuple<JournalMode, Synchronous, LockingMode>> { };

TEST_P(SqliteCreateTest, CreateConnection) {
    auto env = Environment::create(DbType::eSqlite3);
    auto [journal, synchronous, locking] = GetParam();

    std::string name = fmt::format("sqlite/test-{}-{}-{}.db", journal, synchronous, locking);
    ConnectionConfig config = makeSqliteTestDb(name, {
        .journalMode = journal,
        .synchronous = synchronous,
        .lockingMode = locking
    });

    auto conn = getValue(env.tryConnect(config), fmt::format("{}, {}, {}", journal, synchronous, locking));

    GTEST_SUCCEED() << "Created connection";
}

INSTANTIATE_TEST_SUITE_P(SqliteTest, SqliteCreateTest,
    testing::Combine(
        testing::Values(
            JournalMode::eDefault,
            JournalMode::eDelete,
            JournalMode::eTruncate,
            JournalMode::ePersist,
            JournalMode::eMemory,
            JournalMode::eWal,
            JournalMode::eOff
        ),
        testing::Values(
            Synchronous::eDefault,
            Synchronous::eExtra,
            Synchronous::eFull,
            Synchronous::eNormal,
            Synchronous::eOff
        ),
        testing::Values(
            LockingMode::eDefault,
            LockingMode::eRelaxed,
            LockingMode::eExclusive
        )
    ),
    [](const testing::TestParamInfo<SqliteCreateTest::ParamType>& info) {
        return fmt::format("{}_{}_{}", std::get<0>(info.param), std::get<1>(info.param), std::get<2>(info.param));
    }
);

TEST_F(SqliteTest, ErrorHandling) {
    auto conn = env->connect(makeSqliteTestDb("sqlite/errors"));

    // invalid on purpose, should throw
    ASSERT_THROW(conn.updateSql("INSERT INTO not_a_table VALUES (1, 2, 3)"), DbException);

    // after an error, connection should still be usable
    conn.commit().throwIfFailed();

    if (conn.tableExists("test"))
        conn.updateSql("DROP TABLE test");

    conn.updateSql("CREATE TABLE test (id INTEGER, name VARCHAR(100))");

    // check that the database connection is still usable
    conn.updateSql("INSERT INTO test (id, name) VALUES (1, 'test')");

    ResultSet results = conn.selectSql("SELECT * FROM test");
    for (auto row : results) {
        int64 id = getValue(row.getInt(0));
        std::string_view name = getValue(row.getString(1));

        ASSERT_EQ(id, 1);
        ASSERT_EQ(name, "test");
    }
}
