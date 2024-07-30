#include "stdafx.hpp"

#include "common.hpp"

using namespace sm;
using namespace sm::db;

static void checkError(const char *expr, int err) noexcept {
    CTASSERTF(err == SQLITE_OK, "%s = %d (%s)", expr, err, sqlite3_errstr(err));
}

#define CHECK_ERROR(expr) checkError(#expr, expr)

static DbError getError(int err) noexcept {
    int status = [&] {
        switch (err) {
        case SQLITE_OK:
        case SQLITE_ROW:
            return DbError::eOk;

        case SQLITE_DONE:
            return DbError::eNoMoreData;

        default:
            return DbError::eError;
        }
    }();

    return DbError{err, status, sqlite3_errstr(err)};
}

static int execSteps(sqlite3_stmt *stmt) noexcept {
    int err = 0;
    while ((err = sqlite3_step(stmt))) {
        if (err == SQLITE_ROW)
            continue;

        if (err == SQLITE_DONE)
            break;

        CT_NEVER("sqlite3_step = %d (%s)", err, sqlite3_errstr(err));
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

    DbError close() noexcept override {
        int err = sqlite3_finalize(mStatement);
        return getError(err);
    }

    DbError reset() noexcept override {
        int err = sqlite3_reset(mStatement);
        return getError(err);
    }

    DbError bindInt(int index, int64 value) noexcept override {
        int err = sqlite3_bind_int64(mStatement, index, value);
        return getError(err);
    }

    DbError bindBoolean(int index, bool value) noexcept override {
        int err = sqlite3_bind_int(mStatement, index, value ? 1 : 0);
        return getError(err);
    }

    DbError bindString(int index, std::string_view value) noexcept override {
        int err = sqlite3_bind_text(mStatement, index, value.data(), value.size(), SQLITE_STATIC);
        return getError(err);
    }

    DbError bindDouble(int index, double value) noexcept override {
        int err = sqlite3_bind_double(mStatement, index, value);
        return getError(err);
    }

    DbError bindBlob(int index, Blob value) noexcept override {
        int err = sqlite3_bind_blob(mStatement, index, value.data(), value.size_bytes(), SQLITE_STATIC);
        return getError(err);
    }

    DbError bindNull(int index) noexcept override {
        int err = sqlite3_bind_null(mStatement, index);
        return getError(err);
    }

    DbError getBindIndex(std::string_view name, int& index) noexcept override {
        int result = sqlite3_bind_parameter_index(mStatement, name.data());
        index = result;

        return getError((result == 0) ? SQLITE_ERROR : SQLITE_OK);
    }

    DbError select() noexcept override {
        // no-op
        return DbError::ok();
    }

    DbError update(bool commit) noexcept override {
        int err = execSteps(mStatement);
        return getError(err);
    }

    int columnCount() const noexcept override {
        return sqlite3_column_count(mStatement);
    }

    DbError next() noexcept override {
        int err = sqlite3_step(mStatement);
        return getError(err);
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

public:
    SqliteStatement(sqlite3_stmt *stmt) noexcept
        : mStatement(stmt)
    { }
};

class SqliteConnection final : public detail::IConnection {
    sqlite3 *mConnection = nullptr;

    sqlite3_stmt *mBeginStmt = nullptr;
    sqlite3_stmt *mCommitStmt = nullptr;
    sqlite3_stmt *mRollbackStmt = nullptr;

    DbError close() noexcept override {
        CHECK_ERROR(sqlite3_finalize(mBeginStmt));
        CHECK_ERROR(sqlite3_finalize(mCommitStmt));
        CHECK_ERROR(sqlite3_finalize(mRollbackStmt));

        int err = sqlite3_close(mConnection);
        if (err != SQLITE_OK)
            return getError(err);

        mConnection = nullptr;
        return DbError::ok();
    }

    DbError prepare(std::string_view sql, detail::IStatement **statement) noexcept override {
        sqlite3_stmt *stmt = nullptr;
        if (int err = sqlite3_prepare_v2(mConnection, sql.data(), sql.size(), &stmt, nullptr))
            return getError(err);

        *statement = new SqliteStatement{stmt};
        return DbError::ok();
    }

    DbError begin() noexcept override {
        int err = execStatement(mBeginStmt);
        return getError(err);
    }

    DbError commit() noexcept override {
        int err = execStatement(mCommitStmt);
        return getError(err);
    }

    DbError rollback() noexcept override {
        int err = execStatement(mRollbackStmt);
        return getError(err);
    }

    DbError tableExists(std::string_view table, bool& exists) noexcept override {
        detail::IStatement *stmt = nullptr;
        if (DbError err = prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?;", &stmt))
            return err;

        stmt->bindString(0, table);

        if (DbError err = stmt->select())
            return err;

        int64 count = 0;

        if (DbError err = stmt->getInt(0, count))
            return err;

        exists = count > 0;

        if (DbError err = stmt->close())
            return err;

        delete stmt;

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
