#pragma once

#include "drivers/common.hpp"

#include <map>
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

    DbError getErrorInfo(SQLRETURN status, SQLSMALLINT type, SQLHANDLE handle);
    DbError getErrorFromParent(SqlResult result, SQLSMALLINT type, SQLHANDLE parent);

    template<SQLSMALLINT T>
    using WilSqlHandle = wil::unique_any<SQLHANDLE, decltype(&freeSqlHandle<T>), freeSqlHandle<T>>;

    template<SQLSMALLINT T>
    class SqlHandle {
        using WilHandle = WilSqlHandle<T>;

        WilHandle mHandle;
    public:
        static constexpr SQLSMALLINT kHandleType = T;

        SqlHandle(WilHandle handle = SQL_NULL_HANDLE)
            : mHandle(std::move(handle))
        { }

        static SqlHandle create(SQLHANDLE parent) {
            WilHandle handle;
            if (SqlResult result = SQLAllocHandle(kHandleType, parent, &handle))
                getErrorFromParent(result, kHandleType, parent).raise();

            return SqlHandle(std::move(handle));
        }

        DbError getErrorInfo(SQLRETURN status) const noexcept {
            return db2::getErrorInfo(status, kHandleType, mHandle.get());
        }

        operator SQLHANDLE() const noexcept { return mHandle.get(); }
        SQLHANDLE* operator&() noexcept { return mHandle.put(); }
    };

    using SqlStmtHandle = SqlHandle<SQL_HANDLE_STMT>;
    using SqlDbHandle = SqlHandle<SQL_HANDLE_DBC>;
    using SqlEnvHandle = SqlHandle<SQL_HANDLE_ENV>;

    class Db2Statement final : public detail::IStatement {
        SqlStmtHandle mStmtHandle;
        std::string mSqlString;
        std::multimap<std::string, int> mBindIndexLayout;

        DbError start(bool autoCommit, StatementType type) noexcept override;

        DbError execute() noexcept override;

        DbError next() noexcept override;

        std::string getSql() const override;

    public:
        Db2Statement(SqlStmtHandle stmt, std::string sql);
    };

    class Db2Connection final : public detail::IConnection {
        SqlDbHandle mDbHandle;

        /** Execution */

        detail::IStatement *prepare(std::string_view sql) noexcept(false) override;

        /** Transaction */

        DbError begin() noexcept override;
        DbError commit() noexcept override;
        DbError rollback() noexcept override;

        /** Queries */

        std::string setupUserExists() noexcept(false) override;
        std::string setupTableExists() noexcept(false) override;

        DbError setAutoCommit(bool autoCommit) noexcept;
        DbError endTransaction(SQLSMALLINT completionType) noexcept;

    public:
        ~Db2Connection() noexcept override;
        Db2Connection(SqlDbHandle hdbc) noexcept;
    };

    class Db2Environment final : public detail::IEnvironment {
        SqlEnvHandle mEnvHandle;

        bool close() noexcept override { return true; }

        detail::IConnection *connect(const ConnectionConfig& config) noexcept(false) override;

    public:
        Db2Environment(SqlEnvHandle env) noexcept;
    };
}
