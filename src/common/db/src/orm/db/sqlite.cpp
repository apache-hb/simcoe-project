#include "stdafx.hpp"

#include "common.hpp"

#include "core/defer.hpp"

using namespace sm;
using namespace sm::db;

static DataType getColumnType(int type) noexcept {
    switch (type) {
    case SQLITE_INTEGER: return DataType::eInteger;
    case SQLITE_FLOAT: return DataType::eDouble;
    case SQLITE_TEXT: return DataType::eString;
    case SQLITE_BLOB: return DataType::eBlob;
    case SQLITE_NULL: return DataType::eNull;
    default: CT_NEVER("Invalid column type: %d", type);
    }
}

static void checkError(const DbError& err) noexcept {
    CTASSERTF(err.isSuccess(), "Error: %d (%s)", err.code(), err.message().data());
}

#define CHECK_ERROR(expr) checkError(#expr, expr)

static int getStatusType(int err) {
    switch (err) {
    case SQLITE_OK:
    case SQLITE_ROW:
        return DbError::eOk;

    case SQLITE_DONE:
        return DbError::eNoMoreData;

    default:
        return DbError::eError;
    }
}

static DbError getError(int err) noexcept {
    int status = getStatusType(err);

    return DbError{err, status, sqlite3_errstr(err)};
}

static DbError getError(int err, sqlite3 *db) noexcept {
    if (err == SQLITE_OK || err == SQLITE_ROW)
        return DbError::ok();

    if (err == SQLITE_DONE)
        return DbError::noMoreData(err);

    int status = getStatusType(err);
    return DbError{err, status, sqlite3_errmsg(db)};
}

static int execSteps(sqlite3_stmt *stmt) noexcept {
    int err = 0;
    while (true) {
        err = sqlite3_step(stmt);

        if (err == SQLITE_ROW)
            continue;

        if (err == SQLITE_DONE)
            break;

        return err;
    }

    if (err == SQLITE_DONE)
        err = SQLITE_OK;

    return err;
}

static int execStatement(sqlite3_stmt *stmt) noexcept {
    execSteps(stmt);
    return sqlite3_reset(stmt);
}

class SqliteStatement final : public detail::IStatement {
    sqlite3_stmt *mStatement = nullptr;
    int mStatus = SQLITE_OK;

    DbError getStmtError(int err) const noexcept {
        return getError(err, sqlite3_db_handle(mStatement));
    }

public:
    DbError close() noexcept override {
        CTASSERTF(!sqlite3_stmt_busy(mStatement), "Statement is busy");

        sqlite3 *db = sqlite3_db_handle(mStatement);
        int err = sqlite3_finalize(mStatement);
        return getError(err, db);
    }

    DbError reset() noexcept override {
        if (int err = sqlite3_reset(mStatement); err != SQLITE_OK)
            return getStmtError(err);

        if (int err = sqlite3_clear_bindings(mStatement); err != SQLITE_OK)
            return getStmtError(err);

        return DbError::ok();
    }

    int getBindCount() const noexcept override {
        return sqlite3_bind_parameter_count(mStatement);
    }

    DbError getBindIndex(std::string_view name, int& index) const noexcept override {
        std::string id = fmt::format(":{}", name);
        int param = sqlite3_bind_parameter_index(mStatement, id.c_str());
        if (param == 0)
            return getStmtError(SQLITE_ERROR);

        index = param - 1;

        return DbError::ok();
    }

    DbError bindIntByIndex(int index, int64 value) noexcept override {
        int err = sqlite3_bind_int64(mStatement, index + 1, value);
        return getStmtError(err);
    }

    DbError bindBooleanByIndex(int index, bool value) noexcept override {
        int err = sqlite3_bind_int(mStatement, index + 1, value ? 1 : 0);
        return getStmtError(err);
    }

    DbError bindStringByIndex(int index, std::string_view value) noexcept override {
        int err = sqlite3_bind_text(mStatement, index + 1, value.data(), value.size(), SQLITE_STATIC);
        return getStmtError(err);
    }

    DbError bindDoubleByIndex(int index, double value) noexcept override {
        int err = sqlite3_bind_double(mStatement, index + 1, value);
        return getStmtError(err);
    }

    DbError bindBlobByIndex(int index, Blob value) noexcept override {
        int err = sqlite3_bind_blob(mStatement, index + 1, value.data(), value.size_bytes(), SQLITE_STATIC);
        return getStmtError(err);
    }

    DbError bindNullByIndex(int index) noexcept override {
        int err = sqlite3_bind_null(mStatement, index + 1);
        return getStmtError(err);
    }

    DbError select() noexcept override {
        // no-op
        return DbError::ok();
    }

    DbError update(bool autoCommit) noexcept override {
        int err = execSteps(mStatement);
        return getStmtError(err);
    }

    int getColumnCount() const noexcept override {
        return sqlite3_column_count(mStatement);
    }

    DbError getColumnInfo(int index, ColumnInfo& info) const noexcept override {
        const char *name = sqlite3_column_name(mStatement, index);
        int type = sqlite3_column_type(mStatement, index);

        if (name == nullptr)
            return DbError::columnNotFound(std::to_string(index));

        ColumnInfo column = {
            .name = name,
            .type = getColumnType(type),
        };

        info = column;

        return DbError::ok();
    }

    DbError next() noexcept override {
        mStatus = sqlite3_step(mStatement);
        return getStmtError(mStatus);
    }

    DbError getIntByIndex(int index, int64& value) noexcept override {
        CTASSERTF(mStatus == SQLITE_ROW, "Statement is not ready for reading");
        value = sqlite3_column_int64(mStatement, index);
        return DbError::ok();
    }

    DbError getBooleanByIndex(int index, bool& value) noexcept override {
        CTASSERTF(mStatus == SQLITE_ROW, "Statement is not ready for reading");
        value = sqlite3_column_int(mStatement, index) != 0;
        return DbError::ok();
    }

    DbError getStringByIndex(int index, std::string_view& value) noexcept override {
        CTASSERTF(mStatus == SQLITE_ROW, "Statement is not ready for reading");
        const char *text = (const char*)sqlite3_column_text(mStatement, index);
        if (text == nullptr) {
            value = std::string_view{};
        } else {
            value = text;
        }
        return DbError::ok();
    }

    DbError getDoubleByIndex(int index, double& value) noexcept override {
        CTASSERTF(mStatus == SQLITE_ROW, "Statement is not ready for reading");
        value = sqlite3_column_double(mStatement, index);
        return DbError::ok();
    }

    DbError getBlobByIndex(int index, Blob& value) noexcept override {
        CTASSERTF(mStatus == SQLITE_ROW, "Statement is not ready for reading");
        const uint8 *bytes = (const uint8*)sqlite3_column_blob(mStatement, index);
        int length = sqlite3_column_bytes(mStatement, index);

        value = Blob{bytes, bytes + length};

        return DbError::ok();
    }

    SqliteStatement(sqlite3_stmt *stmt) noexcept
        : mStatement(stmt)
    {
        CTASSERT(stmt != nullptr);
    }
};

class SqliteConnection final : public detail::IConnection {
    sqlite3 *mConnection = nullptr;

    sqlite3_stmt *mBeginStmt = nullptr;
    sqlite3_stmt *mCommitStmt = nullptr;
    sqlite3_stmt *mRollbackStmt = nullptr;

    DbError prepare(std::string_view sql, sqlite3_stmt **stmt) noexcept {
        if (int err = sqlite3_prepare_v2(mConnection, sql.data(), sql.size(), stmt, nullptr))
            return getError(err, mConnection);

        return DbError::ok();
    }

    DbError close() noexcept override {
        sqlite3_finalize(mBeginStmt);
        sqlite3_finalize(mCommitStmt);
        sqlite3_finalize(mRollbackStmt);

        int err = sqlite3_close(mConnection);
        if (err != SQLITE_OK)
            return getError(err, mConnection);

        mConnection = nullptr;
        return DbError::ok();
    }

    DbError prepare(std::string_view sql, detail::IStatement **statement) noexcept override {
        sqlite3_stmt *stmt = nullptr;
        if (DbError err = prepare(sql, &stmt))
            return err;

        *statement = new SqliteStatement{stmt};
        return DbError::ok();
    }

    DbError begin() noexcept override {
        int err = execStatement(mBeginStmt);
        return getError(err, mConnection);
    }

    DbError commit() noexcept override {
        int err = execStatement(mCommitStmt);
        return getError(err, mConnection);
    }

    DbError rollback() noexcept override {
        int err = execStatement(mRollbackStmt);
        return getError(err, mConnection);
    }

    DbError tableExists(std::string_view table, bool& exists) noexcept override {
        sqlite3_stmt *stmt = nullptr;
        if (DbError err = prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=:name", &stmt))
            return err;

        SqliteStatement ps{stmt};
        defer { checkError(ps.close()); };

        if (DbError err = ps.bindStringByName("name", table))
            return err;

        int64 count = 0;
        while (ps.next().isSuccess()) {
            if (DbError err = ps.getIntByIndex(0, count))
                return err;
        }

        exists = count > 0;

        return DbError::ok();
    }

    DbError dbVersion(Version& version) const noexcept override {
        const char *name = sqlite3_libversion();
        int num = sqlite3_libversion_number();

        int major = (num >> 16) & 0xFF;
        int minor = (num >> 8) & 0xFF;
        int patch = num & 0xFF;

        version = Version{name, major, minor, patch};

        return DbError::ok();
    }

    static bool isBlankString(const char *text) noexcept {
        while (*text)
            if (!isspace(*text))
                return false;

        return true;
    }

    static void isBlankStringImpl(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
        if (argc != 1)
            return;

        const char *text = (const char*)sqlite3_value_text(argv[0]);
        if (text == nullptr)
            return;

        sqlite3_result_int(ctx, isBlankString(text));
    }

public:
    SqliteConnection(sqlite3 *connection) noexcept
        : mConnection(connection)
    {
        if (int err = sqlite3_prepare_v2(mConnection, "BEGIN;", -1, &mBeginStmt, nullptr))
            CT_NEVER("Failed to prepare BEGIN statement: %s (%d)", sqlite3_errmsg(connection), err);

        if (int err = sqlite3_prepare_v2(mConnection, "COMMIT;", -1, &mCommitStmt, nullptr))
            CT_NEVER("Failed to prepare COMMIT statement: %s (%d)", sqlite3_errmsg(connection), err);

        if (int err = sqlite3_prepare_v2(mConnection, "ROLLBACK;", -1, &mRollbackStmt, nullptr))
            CT_NEVER("Failed to prepare ROLLBACK statement: %s (%d)", sqlite3_errmsg(connection), err);

        int err = sqlite3_create_function(mConnection, "IS_BLANK_STRING", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, isBlankStringImpl, nullptr, nullptr);
        if (err != SQLITE_OK)
            CT_NEVER("Failed to create IS_BLANK_STRING function: %s (%d)", sqlite3_errmsg(connection), err);
    }
};

class SqliteEnvironment final : public detail::IEnvironment {
    DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept override {
        std::string dbPath = std::string{config.host};
        sqlite3 *db = nullptr;
        if (int err = sqlite3_open(dbPath.c_str(), &db))
            return getError(err);

        *connection = new SqliteConnection{db};
        return DbError::ok();
    }

    bool close() noexcept override {
        return false;
    }

public:
    SqliteEnvironment() noexcept = default;
};

DbError detail::sqlite(detail::IEnvironment **env) noexcept {
    static SqliteEnvironment sqlite;
    *env = &sqlite;
    return DbError::ok();
}
