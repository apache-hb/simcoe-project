#include "db2/db2.hpp"

#include "drivers/utils.hpp"

using namespace sm::db;

using Db2Environment = sm::db::db2::Db2Environment;
using Db2Connection = sm::db::db2::Db2Connection;
using SqlResult = sm::db::db2::SqlResult;
using SqlEnvHandle = sm::db::db2::SqlEnvHandle;

static std::string sqlErrorMessage(SQLRETURN result) {
    if (result > 0) {
        return fmt::format("SQL{:0>4}W", result);
    } else {
        return fmt::format("SQL{:0>4}N", std::abs(result));
    }
}

static std::string sqlGetErrorMessage(SQLHENV env, SQLHDBC dbc, SQLHSTMT stmt) {
    SQLCHAR sqlState[SQL_SQLSTATE_SIZE + 1];
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLINTEGER sqlcode;
    SQLSMALLINT length;

    std::vector<std::string> messages;

    while (SQLError(env, dbc, stmt, sqlState, &sqlcode, message, sizeof(message), &length) == SQL_SUCCESS) {
        std::string msg = fmt::format("SQLSTATE: {}, SQLCODE: {}, MESSAGE: {}", (char*)sqlState, sqlcode, (char*)message);
        detail::cleanErrorMessage(msg);
        messages.push_back(msg);
    }

    return fmt::format("{}", fmt::join(messages, "\n"));
}

DbError db2::getSqlError(SQLRETURN result) {
    if (SQL_SUCCEEDED(result))
        return DbError::ok();

    if (result == SQL_NO_DATA)
        return DbError::noData();

    return DbError::error(result, sqlErrorMessage(result));
}

DbError db2::getEnvErrorInfo(SQLRETURN status, SQLHENV env) {
    return DbError::error(status, sqlGetErrorMessage(env, SQL_NULL_HDBC, SQL_NULL_HSTMT));
}

DbError db2::getConnectionErrorInfo(SQLRETURN status, SQLHDBC dbc) {
    return DbError::error(status, sqlGetErrorMessage(SQL_NULL_HENV, dbc, SQL_NULL_HSTMT));
}

DbError db2::getStmtErrorInfo(SQLRETURN status, SQLHSTMT stmt) {
    return DbError::error(status, sqlGetErrorMessage(SQL_NULL_HENV, SQL_NULL_HDBC, stmt));
}

DbError db2::getErrorInfo(SQLRETURN status, SQLSMALLINT type, SQLHANDLE handle) {
    switch (type) {
    case SQL_HANDLE_ENV:
        return db2::getEnvErrorInfo(status, handle);
    case SQL_HANDLE_DBC:
        return db2::getConnectionErrorInfo(status, handle);
    case SQL_HANDLE_STMT:
        return db2::getStmtErrorInfo(status, handle);
    default:
        return db2::getSqlError(status);
    }
}

DbError db2::getErrorFromParent(SqlResult result, SQLSMALLINT type, SQLHANDLE parent) {
    switch (type) {
    case SQL_HANDLE_ENV:
        return db2::getSqlError(result);
    case SQL_HANDLE_DBC:
        return db2::getEnvErrorInfo(result, parent);
    case SQL_HANDLE_STMT:
        return db2::getConnectionErrorInfo(result, parent);
    default:
        return db2::getSqlError(result);
    }
}

detail::IEnvironment *detail::newDb2Environment() {
    SqlEnvHandle henv;
    if (SqlResult result = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv)) {
        if (henv == SQL_NULL_HENV)
            throw DbException{db2::getSqlError(result)};

        throw DbException{db2::getEnvErrorInfo(result, henv)};
    }

    SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    return new db2::Db2Environment(std::move(henv));
}
