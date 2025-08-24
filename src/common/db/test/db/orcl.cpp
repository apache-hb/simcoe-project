#include "test/gtest_common.hpp"

#include "docker/databases/oracle.hpp"
#include "db/db.hpp"
#include "db/environment.hpp"

using namespace std::chrono_literals;
using namespace sm;
using namespace sm::db;

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

class OracleTest : public testing::Test {
    static docker::OracleDbContainer oracledb;
public:
    static Environment env;
    Connection connection;

    static void SetUpTestSuite() {
        if (!Environment::isSupported(DbType::eOracleDB)) {
            GTEST_SKIP() << "OracleDB not supported";
        }

        oracledb = docker::OracleDbContainer({
            .image = docker::OracleDbImage::eOracle23Lite,
        });

        env = Environment::create(DbType::eOracleDB);

        ConnectionConfig config = {
            .port = oracledb.getPort(),
            .host = oracledb.getHost(),
            .user = oracledb.getUsername(),
            .password = oracledb.getPassword(),
            .database = oracledb.getDatabaseName(),
            .role = AssumeRole::eSYSDBA,
        };

        auto admin = getValue(env.tryConnect(config), "Connecting to OracleDB");
        if (!admin.tableSpaceExists("TEST_USER_TBS")) {
            admin.updateSql(R"(CREATE TABLESPACE TEST_USER_TBS
                DATAFILE '/opt/oracle/oradata/FREE/FREEPDB1/test_user_tbs.dbf'
                SIZE 100M;
            )");
        }

        if (!admin.userExists("TEST_USER")) {
            admin.updateSql(R"(CREATE USER TEST_USER IDENTIFIED BY TEST_USER
                DEFAULT TABLESPACE TEST_USER_TBS
                QUOTA 100M ON TEST_USER_TBS;
            )");

            admin.updateSql("GRANT CONNECT, RESOURCE TO TEST_USER");
            admin.updateSql("GRANT CREATE SESSION TO TEST_USER");
            admin.updateSql("GRANT UPDATE ANY TABLE TO TEST_USER");
            admin.updateSql("GRANT DELETE ANY TABLE TO TEST_USER");
            admin.updateSql("GRANT INSERT ANY TABLE TO TEST_USER");
            admin.updateSql("GRANT SELECT ANY TABLE TO TEST_USER");
            admin.updateSql("GRANT CREATE ANY TABLE TO TEST_USER");
        }
    }

    void SetUp() override {
    }
};

#if 0
TEST_CASE("updates") {
    if (!Environment::isSupported(DbType::eOracleDB)) {
        SKIP("OracleDB not supported");
    }

    auto env = Environment::create(DbType::eOracleDB);

    checkOracleTestUser(env);

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

    if (conn.tableExists("test")) {
        checkError(conn.tryUpdateSql("DROP TABLE test"));
        REQUIRE(!conn.tableExists("test"));
    }

    checkError(conn.tryUpdateSql("CREATE TABLE test (id NUMBER, name VARCHAR2(100))"));

    REQUIRE(conn.tableExists("test"));

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
                int64_t id = getValue(results.getInt(0));
                std::string_view name = getValue(results.getString(1));

                REQUIRE(id == 1);
                REQUIRE(name == "bob");

                count++;

                (void)results.next();
            }

            REQUIRE(count == 1);
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
                int64_t id = getValue(row.getInt(0));
                auto name = getValue(row.getString(1));

                REQUIRE(id == 1);
                REQUIRE(name == "test");

                count++;
            }

            REQUIRE(count == 1);
        }
    }

    SECTION("Blob IO") {
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
        CHECK(readBlob.size() == blob.size());

        for (size_t i = 0; i < blob.size(); i++) {
            CHECK(readBlob[i] == blob[i]);
        }

        CHECK(results.next().isDone());
    }
}
#endif
