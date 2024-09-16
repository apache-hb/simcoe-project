#include "stdafx.hpp"

#include "sqlite.hpp"

using namespace sm;
using namespace sm::db;

namespace sqlite = sm::db::detail::sqlite;
using SqliteStatement = sqlite::SqliteStatement;

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

static int doStep(sqlite3_stmt *stmt) noexcept {
    int err = sqlite3_step(stmt);
    fmt::println(stderr, "Step: {} ({}) {}", sqlite3_errstr(err), err, sqlite3_sql(stmt));
    return err;
}

/// Run until either a row is available or the statement is done/errored.
static int runUntilData(sqlite3_stmt *stmt) noexcept {
    int err = doStep(stmt);
    while (err == SQLITE_OK)
        err = doStep(stmt);

    return err;
}

static int runUntilComplete(sqlite3_stmt *stmt) noexcept {
    int err = doStep(stmt);
    while (err == SQLITE_OK || err == SQLITE_ROW)
        err = doStep(stmt);

    return err;
}

int sqlite::execStatement(sqlite3_stmt *stmt) noexcept {
    runUntilComplete(stmt);
    return sqlite3_reset(stmt);
}

DbError SqliteStatement::getStmtError(int err) const noexcept {
    if (err == SQLITE_DONE)
        return DbError::done(err);

    return getError(err, sqlite3_db_handle(mStatement));
}

DbError SqliteStatement::finalize() noexcept {
    sqlite3_finalize(mStatement);
    return DbError::ok();
}

DbError SqliteStatement::start(bool autoCommit, StatementType type) noexcept {
    mStatus = runUntilData(mStatement);
    return getStmtError(mStatus);
}

DbError SqliteStatement::execute() noexcept {
    if (mStatus != SQLITE_DONE)
        runUntilComplete(mStatement);

    sqlite3_reset(mStatement);

    if (int err = sqlite3_clear_bindings(mStatement); err != SQLITE_OK)
        return getStmtError(err);

    return DbError::ok();
}

DbError SqliteStatement::next() noexcept {
    mStatus = doStep(mStatement);
    return getStmtError(mStatus);
}

DbError SqliteStatement::select() noexcept {
    // no-op
    return DbError::ok();
}

DbError SqliteStatement::update(bool autoCommit) noexcept {
    int err = runUntilComplete(mStatement);
    if (err == SQLITE_DONE)
        err = SQLITE_OK;

    return getStmtError(err);
}

int SqliteStatement::getBindCount() const noexcept {
    return sqlite3_bind_parameter_count(mStatement);
}

DbError SqliteStatement::getBindIndex(std::string_view name, int& index) const noexcept {
    std::string id = fmt::format(":{}", name);
    int param = sqlite3_bind_parameter_index(mStatement, id.c_str());
    if (param == 0)
        return getStmtError(SQLITE_ERROR);

    index = param - 1;

    return DbError::ok();
}

DbError SqliteStatement::bindIntByIndex(int index, int64 value) noexcept {
    int err = sqlite3_bind_int64(mStatement, index + 1, value);
    return getStmtError(err);
}

DbError SqliteStatement::bindBooleanByIndex(int index, bool value) noexcept {
    int err = sqlite3_bind_int(mStatement, index + 1, value ? 1 : 0);
    return getStmtError(err);
}

DbError SqliteStatement::bindStringByIndex(int index, std::string_view value) noexcept {
    int err = sqlite3_bind_text(mStatement, index + 1, value.data(), value.size(), SQLITE_STATIC);
    return getStmtError(err);
}

DbError SqliteStatement::bindDoubleByIndex(int index, double value) noexcept {
    int err = sqlite3_bind_double(mStatement, index + 1, value);
    return getStmtError(err);
}

DbError SqliteStatement::bindBlobByIndex(int index, Blob value) noexcept {
    int err = sqlite3_bind_blob(mStatement, index + 1, value.data(), value.size_bytes(), SQLITE_STATIC);
    return getStmtError(err);
}

DbError SqliteStatement::bindNullByIndex(int index) noexcept {
    int err = sqlite3_bind_null(mStatement, index + 1);
    return getStmtError(err);
}

int SqliteStatement::getColumnCount() const noexcept {
    return sqlite3_column_count(mStatement);
}

DbError SqliteStatement::getColumnInfo(int index, ColumnInfo& info) const noexcept {
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

bool SqliteStatement::isRowReady() const noexcept {
    return mStatus == SQLITE_ROW;
}

DbError SqliteStatement::rowNotReady() const noexcept {
    return DbError::notReady(fmt::format("Statement is not ready for reading. {}", sqlite3_errstr(mStatus)));
}

DbError SqliteStatement::getIntByIndex(int index, int64& value) noexcept {
    if (!isRowReady())
        return rowNotReady();

    value = sqlite3_column_int64(mStatement, index);
    return DbError::ok();
}

DbError SqliteStatement::getBooleanByIndex(int index, bool& value) noexcept {
    if (!isRowReady())
        return rowNotReady();

    value = sqlite3_column_int(mStatement, index) != 0;
    return DbError::ok();
}

DbError SqliteStatement::getStringByIndex(int index, std::string_view& value) noexcept {
    if (!isRowReady())
        return rowNotReady();

    if (const char *text = (const char*)sqlite3_column_text(mStatement, index))
        value = text;

    return DbError::ok();
}

DbError SqliteStatement::getDoubleByIndex(int index, double& value) noexcept {
    if (!isRowReady())
        return rowNotReady();

    value = sqlite3_column_double(mStatement, index);
    return DbError::ok();
}

DbError SqliteStatement::getBlobByIndex(int index, Blob& value) noexcept {
    if (!isRowReady())
        return rowNotReady();

    const uint8 *bytes = (const uint8*)sqlite3_column_blob(mStatement, index);
    int length = sqlite3_column_bytes(mStatement, index);

    value = Blob{bytes, bytes + length};

    return DbError::ok();
}

SqliteStatement::SqliteStatement(sqlite3_stmt *stmt) noexcept
    : mStatement(stmt)
{
    CTASSERT(stmt != nullptr);
}
