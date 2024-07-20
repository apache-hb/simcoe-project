#include "stdafx.hpp"

#include "archive/connection.hpp"

using namespace sm;
using namespace sm::db;

Transaction::~Transaction() noexcept {
    sqlite3_finalize(mStmt);
}

std::expected<Connection, DbError> Connection::open(const fs::path& path) noexcept {
    sqlite3 *connection = nullptr;
    if (int result = sqlite3_open(path.string().c_str(), &connection)) {
        return std::unexpected(DbError{result});
    }

    return Connection{connection};
}

void Connection::close() noexcept {
    CTASSERT(mConnection != nullptr);

    int err = sqlite3_close(mConnection);
    CTASSERTF(err == SQLITE_OK, "sqlite3_close failed: %s", sqlite3_errstr(err));
    mConnection = nullptr;
}

std::string_view db::toString(DbError error) noexcept {
    return sqlite3_errstr((int)error);
}

std::expected<Transaction, DbError> Connection::execute(std::string_view sql) noexcept {
    sqlite3_stmt *iter = nullptr;
    if (int result = sqlite3_prepare_v2(mConnection, sql.data(), sql.size(), &iter, nullptr)) {
        sqlite3_finalize(iter);
        return std::unexpected(DbError{result});
    }

    while (sqlite3_step(iter) == SQLITE_ROW) {

    }

    return Transaction{iter};
}

std::expected<Query, DbError> Connection::select(std::string_view sql) noexcept {
    sqlite3_stmt *iter = nullptr;
    if (int result = sqlite3_prepare_v2(mConnection, sql.data(), sql.size(), &iter, nullptr)) {
        sqlite3_finalize(iter);
        return std::unexpected(DbError{result});
    }

    return Query{iter};
}

QueryIterator& QueryIterator::operator++() noexcept {
    int result = sqlite3_step(mTransaction->getStmt());
    if (result != SQLITE_ROW) {
        mTransaction = nullptr;
    }

    return *this;
}

ResultSet QueryIterator::operator*() const noexcept {
    return ResultSet{*mTransaction};
}

BindPoint Stmt::set(std::string_view name) noexcept {
    int index = sqlite3_bind_parameter_index(mStmt, name.data());
    CTASSERTF(index > 0, "sqlite3_bind_parameter_index failed: %s", sqlite3_errstr(index));

    return BindPoint{mStmt, index};
}

void BindPoint::to(int64 value) noexcept {
    int result = sqlite3_bind_int64(mStmt, mIndex, value);
    CTASSERTF(result == SQLITE_OK, "sqlite3_bind_int64 failed: %s", sqlite3_errstr(result));
}

void BindPoint::to(std::string_view value) noexcept {
    int result = sqlite3_bind_text(mStmt, mIndex, value.data(), value.size(), SQLITE_STATIC);
    CTASSERTF(result == SQLITE_OK, "sqlite3_bind_text failed: %s", sqlite3_errstr(result));
}

void BindPoint::to(double value) noexcept {
    int result = sqlite3_bind_double(mStmt, mIndex, value);
    CTASSERTF(result == SQLITE_OK, "sqlite3_bind_double failed: %s", sqlite3_errstr(result));
}

void BindPoint::to(std::nullptr_t) noexcept {
    int result = sqlite3_bind_null(mStmt, mIndex);
    CTASSERTF(result == SQLITE_OK, "sqlite3_bind_null failed: %s", sqlite3_errstr(result));
}
