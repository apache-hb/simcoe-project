#include "db2/db2.hpp"

using namespace sm::db;

using Db2Environment = sm::db::db2::Db2Environment;
using Db2Connection = sm::db::db2::Db2Connection;

DbError Db2Environment::connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept {
    SqlDbHandleEx hdbc = SqlDbHandleEx::create(mEnvHandle);

    SQLPOINTER autocommit = config.autoCommit ? (SQLPOINTER)SQL_AUTOCOMMIT_ON : (SQLPOINTER)SQL_AUTOCOMMIT_OFF;
    if (SqlResult status = SQLSetConnectAttr(hdbc, SQL_AUTOCOMMIT, autocommit, 0))
        return db2::getConnectionErrorInfo(status, hdbc);

    uintptr_t timeout = static_cast<uintptr_t>(config.timeout.count());
    if (SqlResult status = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)timeout, 0))
        return db2::getConnectionErrorInfo(status, hdbc);

    SQLCHAR *szDSN = (SQLCHAR*)config.database.data();
    SQLCHAR *szUID = (SQLCHAR*)config.user.data();
    SQLCHAR *szAuth = (SQLCHAR*)config.password.data();

    SQLSMALLINT cbDSN = (SQLSMALLINT)config.database.size();
    SQLSMALLINT cbUID = (SQLSMALLINT)config.user.size();
    SQLSMALLINT cbAuth = (SQLSMALLINT)config.password.size();

    if (SqlResult result = SQLConnect(hdbc, szDSN, cbDSN, szUID, cbUID, szAuth, cbAuth)) {
        return (hdbc != SQL_NULL_HDBC)
            ? db2::getConnectionErrorInfo(result, hdbc)
            : db2::getEnvErrorInfo(result, mEnvHandle);
    }

    *connection = new Db2Connection(std::move(hdbc));

    return DbError::ok();
}

Db2Environment::Db2Environment(SqlEnvHandleEx env) noexcept
    : mEnvHandle(std::move(env))
{ }
