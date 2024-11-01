#include "db2/db2.hpp"

using namespace sm::db;

using Db2Environment = sm::db::db2::Db2Environment;
using Db2Connection = sm::db::db2::Db2Connection;

detail::IConnection *Db2Environment::connect(const ConnectionConfig& config) noexcept(false) {
    SqlDbHandleEx hdbc = SqlDbHandleEx::create(mEnvHandle);

    SQLPOINTER autocommit = config.autoCommit ? (SQLPOINTER)SQL_AUTOCOMMIT_ON : (SQLPOINTER)SQL_AUTOCOMMIT_OFF;
    if (SqlResult status = SQLSetConnectAttr(hdbc, SQL_AUTOCOMMIT, autocommit, 0))
        throw DbConnectionException{hdbc.getErrorInfo(status), config};

    uintptr_t timeout = static_cast<uintptr_t>(config.timeout.count());
    if (SqlResult status = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)timeout, 0))
        throw DbConnectionException{hdbc.getErrorInfo(status), config};

    SQLCHAR *szDSN = (SQLCHAR*)config.database.data();
    SQLCHAR *szUID = (SQLCHAR*)config.user.data();
    SQLCHAR *szAuth = (SQLCHAR*)config.password.data();

    SQLSMALLINT cbDSN = (SQLSMALLINT)config.database.size();
    SQLSMALLINT cbUID = (SQLSMALLINT)config.user.size();
    SQLSMALLINT cbAuth = (SQLSMALLINT)config.password.size();

    if (SqlResult result = SQLConnect(hdbc, szDSN, cbDSN, szUID, cbUID, szAuth, cbAuth)) {
        DbError error = (hdbc != SQL_NULL_HDBC)
            ? hdbc.getErrorInfo(result)
            : mEnvHandle.getErrorInfo(result);

        throw DbConnectionException{error, config};
    }

    return new Db2Connection(std::move(hdbc));
}

Db2Environment::Db2Environment(SqlEnvHandleEx env) noexcept
    : mEnvHandle(std::move(env))
{ }
