#pragma once

#include "drivers/common.hpp"

#include <sqlcli1.h>

#include <wil/resource.h>

namespace sm::db::db2 {
    DbError getSqlError(SQLRETURN result);
    DbError getEnvErrorInfo(SQLRETURN status, SQLHENV env);
    DbError getConnectionErrorInfo(SQLRETURN status, SQLHDBC dbc);
    DbError getStmtErrorInfo(SQLRETURN status, SQLHSTMT stmt);

    template<SQLSMALLINT T>
    void freeSqlHandle(SQLHANDLE handle) noexcept {
        if (handle != SQL_NULL_HANDLE)
            SQLFreeHandle(T, handle);
    }

    class SqlResult {
        SQLRETURN mResult;

    public:
        SqlResult(SQLRETURN result) noexcept
            : mResult(result)
        { }

        operator SQLRETURN() const noexcept {
            return mResult;
        }

        operator bool() const noexcept {
            return !SQL_SUCCEEDED(mResult);
        }
    };

    template<SQLSMALLINT T>
    using SqlHandle = wil::unique_any<SQLHANDLE, decltype(&freeSqlHandle<T>), freeSqlHandle<T>>;

    using SqlStmtHandle = SqlHandle<SQL_HANDLE_STMT>;
    using SqlDbHandle = SqlHandle<SQL_HANDLE_DBC>;
    using SqlEnvHandle = SqlHandle<SQL_HANDLE_ENV>;

    class Db2Statement final : public detail::IStatement {
        SqlStmtHandle mStmtHandle;

        DbError finalize() noexcept override;

        DbError start(bool autoCommit, StatementType type) noexcept override;

        DbError execute() noexcept override;

        DbError next() noexcept override;

    public:
        Db2Statement(SqlStmtHandle stmt) noexcept;
    };

    class Db2Connection final : public detail::IConnection {
        SqlDbHandle mDbHandle;
        Version mClientVersion;
        Version mServerVersion;

        /** Lifecycle */

        DbError close() noexcept override;

        /** Execution */

        DbError prepare(std::string_view sql, detail::IStatement **stmt) noexcept override;

        /** Transaction */

        DbError begin() noexcept override;
        DbError commit() noexcept override;
        DbError rollback() noexcept override;

        /** Queries */

        std::string setupTableExists() noexcept(false) override;

        Version clientVersion() const noexcept override;
        Version serverVersion() const noexcept override;

        bool hasNamedPlaceholders() const noexcept override { return false; }
        bool hasCommentOn() const noexcept override { return true; }

        DbError setAutoCommit(bool autoCommit) noexcept;
        DbError endTransaction(SQLSMALLINT completionType) noexcept;

        Db2Connection(SqlDbHandle hdbc, bool hasGetInfoSupport) noexcept;
    public:
        Db2Connection(SqlDbHandle hdbc) noexcept;
    };

    class Db2Environment final : public detail::IEnvironment {
        SqlEnvHandle mEnvHandle;

        bool close() noexcept override { return true; }

        DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept override;

    public:
        Db2Environment(SqlEnvHandle env) noexcept;
    };
}
