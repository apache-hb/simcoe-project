#include "db2/db2.hpp"

using namespace sm::db;

using Db2Environment = sm::db::db2::Db2Environment;
using Db2Connection = sm::db::db2::Db2Connection;

DbError Db2Environment::connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept {
    SqlDbHandle hdbc;
    if (SqlResult result = SQLAllocHandle(SQL_HANDLE_DBC, mEnvHandle.get(), &hdbc))
        return getEnvErrorInfo(result, mEnvHandle.get());

    SQLPOINTER autocommit = config.autoCommit ? (SQLPOINTER)SQL_AUTOCOMMIT_ON : (SQLPOINTER)SQL_AUTOCOMMIT_OFF;
    if (SqlResult status = SQLSetConnectAttr(hdbc.get(), SQL_AUTOCOMMIT, autocommit, 0))
        return db2::getConnectionErrorInfo(status, hdbc.get());

    uintptr_t timeout = static_cast<uintptr_t>(config.timeout.count());
    if (SqlResult status = SQLSetConnectAttr(hdbc.get(), SQL_LOGIN_TIMEOUT, (SQLPOINTER)timeout, 0))
        return db2::getConnectionErrorInfo(status, hdbc.get());

    SQLCHAR *szDSN = (SQLCHAR*)config.database.data();
    SQLCHAR *szUID = (SQLCHAR*)config.user.data();
    SQLCHAR *szAuth = (SQLCHAR*)config.password.data();

    SQLSMALLINT cbDSN = (SQLSMALLINT)config.database.size();
    SQLSMALLINT cbUID = (SQLSMALLINT)config.user.size();
    SQLSMALLINT cbAuth = (SQLSMALLINT)config.password.size();

    if (SqlResult result = SQLConnect(hdbc.get(), szDSN, cbDSN, szUID, cbUID, szAuth, cbAuth)) {
        return (hdbc != SQL_NULL_HDBC)
            ? db2::getConnectionErrorInfo(result, hdbc.get())
            : db2::getEnvErrorInfo(result, mEnvHandle.get());
    }

    fmt::println(stderr, "HDBC: {}", (void*)hdbc.get());

    *connection = new Db2Connection(std::move(hdbc));

    return DbError::ok();
}

Db2Environment::Db2Environment(SqlEnvHandle env) noexcept
    : mEnvHandle(std::move(env))
{ }
