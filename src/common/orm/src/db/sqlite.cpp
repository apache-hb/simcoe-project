#include "stdafx.hpp"

#include "common.hpp"

using namespace sm;
using namespace sm::db;

static DataType getColumnType(int type) noexcept {
    switch (type) {
    case SQLITE_INTEGER: return DataType::eInteger;
    case SQLITE_FLOAT: return DataType::eDouble;
    case SQLITE_TEXT: return DataType::eString;
    case SQLITE_BLOB: return DataType::eBlob;
    case SQLITE_NULL: return DataType::eNull;
    default: return DataType::eNull;
    }
}

static void checkError(const char *expr, int err) noexcept {
    CTASSERTF(err == SQLITE_OK, "%s = %d (%s)", expr, err, sqlite3_errstr(err));
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
    int err = sqlite3_reset(stmt);
    return err;
}

class SqliteStatement final : public detail::IStatement {
    sqlite3_stmt *mStatement = nullptr;

    DbError getStmtError(int err) const noexcept {
        return getError(err, sqlite3_db_handle(mStatement));
    }

public:
    DbError close() noexcept override {
        sqlite3 *db = sqlite3_db_handle(mStatement);
        int err = sqlite3_finalize(mStatement);
        return getError(err, db);
    }

    DbError reset() noexcept override {
        int err = sqlite3_reset(mStatement);
        return getStmtError(err);
    }

    DbError bindInt(int index, int64 value) noexcept override {
        int err = sqlite3_bind_int64(mStatement, index + 1, value);
        return getStmtError(err);
    }

    DbError bindBoolean(int index, bool value) noexcept override {
        int err = sqlite3_bind_int(mStatement, index + 1, value ? 1 : 0);
        return getStmtError(err);
    }

    DbError bindString(int index, std::string_view value) noexcept override {
        int err = sqlite3_bind_text(mStatement, index + 1, value.data(), value.size(), SQLITE_STATIC);
        return getStmtError(err);
    }

    DbError bindDouble(int index, double value) noexcept override {
        int err = sqlite3_bind_double(mStatement, index + 1, value);
        return getStmtError(err);
    }

    DbError bindBlob(int index, Blob value) noexcept override {
        int err = sqlite3_bind_blob(mStatement, index + 1, value.data(), value.size_bytes(), SQLITE_STATIC);
        return getStmtError(err);
    }

    DbError bindNull(int index) noexcept override {
        int err = sqlite3_bind_null(mStatement, index + 1);
        return getStmtError(err);
    }

    std::expected<int, DbError> getBindIndex(std::string_view name) const noexcept {
        std::string id = fmt::format(":{}", name);
        int index = sqlite3_bind_parameter_index(mStatement, id.c_str());
        if (index == 0)
            return std::unexpected(getStmtError(SQLITE_ERROR));

        return index;
    }

    DbError bindInt(std::string_view name, int64 value) noexcept override {
        int index = TRY_UNWRAP(getBindIndex(name));
        return bindInt(index - 1, value);
    }

    DbError bindBoolean(std::string_view name, bool value) noexcept override {
        int index = TRY_UNWRAP(getBindIndex(name));
        return bindBoolean(index - 1, value);
    }

    DbError bindString(std::string_view name, std::string_view value) noexcept override {
        int index = TRY_UNWRAP(getBindIndex(name));
        return bindString(index - 1, value);
    }

    DbError bindDouble(std::string_view name, double value) noexcept override {
        int index = TRY_UNWRAP(getBindIndex(name));
        return bindDouble(index - 1, value);
    }

    DbError bindBlob(std::string_view name, Blob value) noexcept override {
        int index = TRY_UNWRAP(getBindIndex(name));
        return bindBlob(index - 1, value);
    }

    DbError bindNull(std::string_view name) noexcept override {
        int index = TRY_UNWRAP(getBindIndex(name));
        return bindNull(index - 1);
    }

    DbError select() noexcept override {
        // no-op
        return DbError::ok();
    }

    DbError update(bool autoCommit) noexcept override {
        int err = execSteps(mStatement);
        return getStmtError(err);
    }

    int columnCount() const noexcept override {
        return sqlite3_column_count(mStatement);
    }

    Column column(int index) const noexcept override {
        const char *name = sqlite3_column_name(mStatement, index);
        int type = sqlite3_column_type(mStatement, index);

        return Column{name, getColumnType(type)};
    }

    DbError next() noexcept override {
        int err = sqlite3_step(mStatement);
        return getStmtError(err);
    }

    DbError getInt(int index, int64& value) noexcept override {
        value = sqlite3_column_int64(mStatement, index);
        return DbError::ok();
    }

    DbError getBoolean(int index, bool& value) noexcept override {
        value = sqlite3_column_int(mStatement, index) != 0;
        return DbError::ok();
    }

    DbError getString(int index, std::string_view& value) noexcept override {
        value = (const char*)sqlite3_column_text(mStatement, index);
        return DbError::ok();
    }

    DbError getDouble(int index, double& value) noexcept override {
        value = sqlite3_column_double(mStatement, index);
        return DbError::ok();
    }

    DbError getBlob(int index, Blob& value) noexcept override {
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
        CHECK_ERROR(sqlite3_finalize(mBeginStmt));
        CHECK_ERROR(sqlite3_finalize(mCommitStmt));
        CHECK_ERROR(sqlite3_finalize(mRollbackStmt));

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
        return getError(err);
    }

    DbError tableExists(std::string_view table, bool& exists) noexcept override {
        sqlite3_stmt *stmt = nullptr;
        if (DbError err = prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=:name", &stmt))
            return err;

        SqliteStatement ps{stmt};

        if (DbError err = ps.bindString("name", table))
            return err;

        int64 count = 0;
        while (ps.next().isSuccess()) {
            if (DbError err = ps.getInt(0, count))
                return err;
        }

        if (DbError err = ps.close())
            return err;

        exists = count > 0;

        return DbError::ok();
    }

public:
    SqliteConnection(sqlite3 *connection) noexcept
        : mConnection(connection)
    {
        if (int err = sqlite3_prepare_v2(mConnection, "BEGIN;", -1, &mBeginStmt, nullptr))
            CT_NEVER("Failed to prepare BEGIN statement: %s", sqlite3_errstr(err));

        if (int err = sqlite3_prepare_v2(mConnection, "COMMIT;", -1, &mCommitStmt, nullptr))
            CT_NEVER("Failed to prepare COMMIT statement: %s", sqlite3_errstr(err));

        if (int err = sqlite3_prepare_v2(mConnection, "ROLLBACK;", -1, &mRollbackStmt, nullptr))
            CT_NEVER("Failed to prepare ROLLBACK statement: %s", sqlite3_errstr(err));
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
