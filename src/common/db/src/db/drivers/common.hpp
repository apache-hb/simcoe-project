#pragma once

#include "db/core.hpp"
#include "db/error.hpp"

#include "dao/dao.hpp"

namespace sm::db::detail {
    struct IEnvironment {
        virtual ~IEnvironment() = default;

        /** Lifecycle */

        virtual bool close() noexcept = 0;

        /** Connection */

        virtual DbError connect(const ConnectionConfig& config, IConnection **connection) noexcept = 0;
    };

    struct IConnection {
        virtual ~IConnection() = default;

        /** Lifecycle */

        virtual DbError close() noexcept = 0;

        /** Execution */

        virtual DbError prepare(std::string_view sql, IStatement **stmt) noexcept = 0;

        /** Transaction */

        virtual DbError begin() noexcept = 0;
        virtual DbError commit() noexcept = 0;
        virtual DbError rollback() noexcept = 0;

        /** Insert */

        virtual DbError setupInsert(const dao::TableInfo& table, std::string& sql) noexcept {
            return DbError::todo();
        }

        virtual DbError setupInsertReturningPrimaryKey(const dao::TableInfo& table, std::string& sql) noexcept {
            return DbError::todo();
        }

        /** misc */

        virtual DbError tableExists(std::string_view name, bool& exists) noexcept {
            return DbError::todo();
        }

        virtual DbError createTable(const dao::TableInfo& table) noexcept {
            return DbError::todo();
        }

        virtual DbError dbVersion(Version& version) const noexcept {
            version = Version { "unknown", 0, 0, 0 };
            return DbError::ok();
        }
    };

    struct IStatement {
        virtual ~IStatement() = default;

        /** Lifecycle */

        virtual DbError close() noexcept = 0;
        virtual DbError reset() noexcept = 0;

        /** Execution */

        virtual DbError select() noexcept = 0;
        virtual DbError update(bool autoCommit) noexcept = 0;

        /** Fetch results */

        virtual int getColumnCount() const noexcept = 0;

        virtual DbError getColumnIndex(std::string_view name, int& index) const noexcept {
            return DbError::todo();
        }

        virtual DbError getColumnInfo(int index, ColumnInfo& info) const noexcept {
            return DbError::todo();
        }

        virtual DbError next() noexcept = 0;

        virtual DbError getIntByIndex(int index, int64& value) noexcept = 0;
        virtual DbError getBooleanByIndex(int index, bool& value) noexcept = 0;
        virtual DbError getStringByIndex(int index, std::string_view& value) noexcept = 0;
        virtual DbError getDoubleByIndex(int index, double& value) noexcept = 0;
        virtual DbError getBlobByIndex(int index, Blob& value) noexcept = 0;

        virtual DbError getIntByName(std::string_view column, int64& value) noexcept;
        virtual DbError getBooleanByName(std::string_view column, bool& value) noexcept;
        virtual DbError getStringByName(std::string_view column, std::string_view& value) noexcept;
        virtual DbError getDoubleByName(std::string_view column, double& value) noexcept;
        virtual DbError getBlobByName(std::string_view column, Blob& value) noexcept;


        /** Binding */

        virtual int getBindCount() const noexcept = 0;

        virtual DbError getBindIndex(std::string_view name, int& index) const noexcept {
            return DbError::todo();
        }

        virtual DbError getBindInfo(int index, BindInfo& info) const noexcept {
            return DbError::todo();
        }


        virtual DbError bindIntByIndex(int index, int64 value) noexcept = 0;
        virtual DbError bindBooleanByIndex(int index, bool value) noexcept = 0;
        virtual DbError bindStringByIndex(int index, std::string_view value) noexcept = 0;
        virtual DbError bindDoubleByIndex(int index, double value) noexcept = 0;
        virtual DbError bindBlobByIndex(int index, Blob value) noexcept = 0;
        virtual DbError bindNullByIndex(int index) noexcept = 0;

        virtual DbError bindIntByName(std::string_view name, int64 value) noexcept;
        virtual DbError bindBooleanByName(std::string_view name, bool value) noexcept;
        virtual DbError bindStringByName(std::string_view name, std::string_view value) noexcept;
        virtual DbError bindDoubleByName(std::string_view name, double value) noexcept;
        virtual DbError bindBlobByName(std::string_view name, Blob value) noexcept;
        virtual DbError bindNullByName(std::string_view name) noexcept;

        DbError findColumnIndex(std::string_view name, int& index) const noexcept;
        DbError findBindIndex(std::string_view name, int& index) const noexcept;

    private:
        template<typename T>
        DbError bindValue(std::string_view name, T value, DbError (IStatement::*func)(int, T)) noexcept {
            int index = -1;
            if (DbError error = findBindIndex(name, index))
                return error;

            return (this->*func)(index, value);
        }

        template<typename T>
        DbError getValue(std::string_view name, T& value, DbError (IStatement::*func)(int, T&)) noexcept {
            int index = -1;
            if (DbError error = findColumnIndex(name, index))
                return error;

            return (this->*func)(index, value);
        }
    };

    DbError getSqliteEnv(IEnvironment **env) noexcept;
    DbError postgres(IEnvironment **env) noexcept;
    DbError mysql(IEnvironment **env) noexcept;
    DbError getOracleEnv(IEnvironment **env) noexcept;
    DbError mssql(IEnvironment **env) noexcept;
    DbError db2(IEnvironment **env) noexcept;
}
