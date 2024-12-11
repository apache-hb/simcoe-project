#include "stdafx.hpp"

#include "sqlite/sqlite.hpp"
#include "sqlite/statement.hpp"

using namespace sm;
using namespace sm::db;

namespace chrono = std::chrono;

using SqliteStatement = sqlite::SqliteStatement;

static DataType getColumnType(int type) noexcept {
    switch (type) {
    case SQLITE_INTEGER: return DataType::eInteger;
    case SQLITE_FLOAT: return DataType::eDouble;
    case SQLITE_TEXT: return DataType::eString;
    case SQLITE_BLOB: return DataType::eBlob;
    case SQLITE_NULL: return DataType::eNull;
    default: return DataType::eUnknown;
    }
}

static int doStep(sqlite3_stmt *stmt) noexcept {
    int err = sqlite3_step(stmt);
    // fmt::println(stderr, "sqlite step: {} ({}) {}", sqlite3_errstr(err), err, sqlite3_sql(stmt));
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

void sqlite::CloseStmt::operator()(sqlite3_stmt *stmt) noexcept {
    sqlite3_finalize(stmt);
}

void *SqliteStatement::addBoundData(Blob data) {
    DataHolder& holder = mBoundData.emplace_front(std::move(data));
    return std::visit([](auto& data) -> void* { return (void*)data.data(); }, holder);
}

const char *SqliteStatement::addBoundData(std::string_view data) {
    DataHolder& holder = mBoundData.emplace_front(std::string(data));
    return std::visit([](auto& data) -> const char * { return (const char *)data.data(); }, holder);
}

DbError SqliteStatement::getStmtError(int err) const noexcept {
    if (err == SQLITE_DONE) {
        return DbError::done(err);
    }

    return getError(err, sqlite3_db_handle(mStatement.get()));
}

DbError SqliteStatement::getExecuteResult(int status) noexcept {
    if (status == SQLITE_DONE) {
        mRowsAffected = sqlite3_changes64(sqlite3_db_handle(mStatement.get()));
        return DbError::done(status);
    }

    return getError(status, sqlite3_db_handle(mStatement.get()));
}

DbError SqliteStatement::start(bool autoCommit, StatementType type) noexcept {
    mStatus = runUntilData(mStatement.get());
    return getExecuteResult(mStatus);
}

DbError SqliteStatement::execute() noexcept {
    if (mStatus != SQLITE_DONE)
        runUntilComplete(mStatement.get());

    mRowsAffected = sqlite3_changes64(sqlite3_db_handle(mStatement.get()));
    sqlite3_reset(mStatement.get());

    if (int err = sqlite3_clear_bindings(mStatement.get()); err != SQLITE_OK)
        return getStmtError(err);

    return DbError::ok();
}

DbError SqliteStatement::next() noexcept {
    mStatus = doStep(mStatement.get());
    return getExecuteResult(mStatus);
}

std::string SqliteStatement::getSql() const {
    return sqlite3_sql(mStatement.get());
}

int SqliteStatement::getBindCount() const noexcept {
    return sqlite3_bind_parameter_count(mStatement.get());
}

int64_t SqliteStatement::getRowsAffected() const noexcept {
    return mRowsAffected;
}

DbError SqliteStatement::getBindIndex(std::string_view name, int& index) const noexcept {
    std::string id = fmt::format(":{}", name);
    int param = sqlite3_bind_parameter_index(mStatement.get(), id.c_str());
    if (param == 0)
        return getStmtError(SQLITE_ERROR);

    index = param - 1;

    return DbError::ok();
}

DbError SqliteStatement::bindIntByIndex(int index, int64_t value) noexcept {
    int err = sqlite3_bind_int64(mStatement.get(), index + 1, value);
    return getStmtError(err);
}

DbError SqliteStatement::bindBooleanByIndex(int index, bool value) noexcept {
    int err = sqlite3_bind_int(mStatement.get(), index + 1, value ? 1 : 0);
    return getStmtError(err);
}

DbError SqliteStatement::bindStringByIndex(int index, std::string_view value) noexcept {
    size_t size = value.size();
    const char *data = addBoundData(value);
    int err = sqlite3_bind_text(mStatement.get(), index + 1, data, size, SQLITE_STATIC);
    return getStmtError(err);
}

DbError SqliteStatement::bindDoubleByIndex(int index, double value) noexcept {
    int err = sqlite3_bind_double(mStatement.get(), index + 1, value);
    return getStmtError(err);
}

DbError SqliteStatement::bindBlobByIndex(int index, Blob value) noexcept {
    size_t size = value.size();
    void *data = addBoundData(std::move(value));
    int err = sqlite3_bind_blob(mStatement.get(), index + 1, data, size, SQLITE_STATIC);
    return getStmtError(err);
}

DbError SqliteStatement::bindDateTimeByIndex(int index, DateTime value) noexcept {
    int64_t timestamp = chrono::duration_cast<chrono::milliseconds>(value.time_since_epoch()).count();
    int err = sqlite3_bind_int64(mStatement.get(), index + 1, timestamp);
    return getStmtError(err);
}

DbError SqliteStatement::bindNullByIndex(int index) noexcept {
    int err = sqlite3_bind_null(mStatement.get(), index + 1);
    return getStmtError(err);
}

int SqliteStatement::getColumnCount() const noexcept {
    return sqlite3_column_count(mStatement.get());
}

DbError SqliteStatement::getColumnIndex(std::string_view name, int& index) const noexcept {
    for (int i = 0; i < getColumnCount(); i++) {
        if (name == sqlite3_column_name(mStatement.get(), i)) {
            index = i;
            return DbError::ok();
        }
    }

    return DbError::columnNotFound(name);
}

DbError SqliteStatement::getColumnInfo(int index, ColumnInfo& info) const noexcept {
    const char *name = sqlite3_column_name(mStatement.get(), index);
    int type = sqlite3_column_type(mStatement.get(), index);

    if (name == nullptr)
        return DbError::columnNotFound(std::to_string(index));

    ColumnInfo column = {
        .name = name,
        .type = getColumnType(type),
    };

    info = column;

    return DbError::ok();
}

DbError SqliteStatement::getColumnInfo(std::string_view name, ColumnInfo& info) const noexcept {
    int index;
    if (DbError error = getColumnIndex(name, index))
        return error;

    return getColumnInfo(index, info);
}

DbError SqliteStatement::getIntByIndex(int index, int64_t& value) noexcept {
    value = sqlite3_column_int64(mStatement.get(), index);
    return DbError::ok();
}

DbError SqliteStatement::getBooleanByIndex(int index, bool& value) noexcept {
    value = sqlite3_column_int(mStatement.get(), index) != 0;
    return DbError::ok();
}

DbError SqliteStatement::getStringByIndex(int index, std::string_view& value) noexcept {
    if (const char *text = (const char*)sqlite3_column_text(mStatement.get(), index))
        value = text;

    return DbError::ok();
}

DbError SqliteStatement::getDoubleByIndex(int index, double& value) noexcept {
    value = sqlite3_column_double(mStatement.get(), index);
    return DbError::ok();
}

DbError SqliteStatement::getDateTimeByIndex(int index, DateTime& value) noexcept {
    int64_t timestamp = sqlite3_column_int64(mStatement.get(), index);
    value = DateTime{chrono::milliseconds{timestamp}};
    return DbError::ok();
}

DbError SqliteStatement::getBlobByIndex(int index, Blob& value) noexcept {
    const std::uint8_t *bytes = (const std::uint8_t*)sqlite3_column_blob(mStatement.get(), index);
    int length = sqlite3_column_bytes(mStatement.get(), index);
    if (length > 0) {
        value = Blob{bytes, bytes + length};
    }

    return DbError::ok();
}

DbError SqliteStatement::isNullByIndex(int index, bool& value) noexcept {
    value = sqlite3_column_type(mStatement.get(), index) == SQLITE_NULL;
    return DbError::ok();
}

DbError SqliteStatement::isNullByName(std::string_view column, bool& value) noexcept {
    int index;
    if (DbError error = getColumnIndex(column, index))
        return error;

    return isNullByIndex(index, value);
}

SqliteStatement::SqliteStatement(sqlite3_stmt *stmt) noexcept
    : mStatement(stmt)
{
    CTASSERT(stmt != nullptr);
}
