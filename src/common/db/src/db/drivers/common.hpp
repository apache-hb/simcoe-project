#pragma once

#include "db/core.hpp"
#include "db/error.hpp"

#include "dao/dao.hpp"

#include "logs/logs.hpp"

namespace sm::db {
    LOG_CATEGORY(gLog);
}

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

        virtual DbError setupInsertOrUpdate(const dao::TableInfo& table, std::string& sql) noexcept {
            return DbError::todo();
        }

        virtual DbError setupInsertReturningPrimaryKey(const dao::TableInfo& table, std::string& sql) noexcept {
            return DbError::todo();
        }

        /** Select */

        virtual DbError setupSelect(const dao::TableInfo& table, std::string& sql) noexcept {
            return DbError::todo();
        }

        /** Update */

        virtual DbError setupUpdate(const dao::TableInfo& table, std::string& sql) noexcept {
            return DbError::todo();
        }

        /** Triggers */

        virtual DbError setupSingletonTrigger(const dao::TableInfo& table, std::string& sql) noexcept {
            return DbError::todo();
        }

        /** Tables */

        virtual DbError setupTableExists(std::string& sql) noexcept {
            return DbError::todo();
        }

        virtual DbError tableExists(std::string_view name, bool& exists) noexcept;

        virtual DbError createTable(const dao::TableInfo& table) noexcept {
            return DbError::todo();
        }

        /** Version */

        virtual DbError clientVersion(Version& version) const noexcept {
            version = Version { "unknown", 0, 0, 0 };
            return DbError::ok();
        }

        virtual DbError serverVersion(Version& version) const noexcept {
            version = Version { "unknown", 0, 0, 0 };
            return DbError::ok();
        }
    };

    struct IStatement {
        virtual ~IStatement() = default;

        /** Lifecycle */

        virtual DbError finalize() noexcept {
            return DbError::todo();
        }

        /** Execution */

        /** Execute statement and fetch first row of data */
        virtual DbError start(bool autoCommit, StatementType type) noexcept {
            return DbError::todo();
        }

        /** Complete execution of statement */
        virtual DbError execute() noexcept {
            return DbError::todo();
        }

        /** Get next row of data */
        virtual DbError next() noexcept {
            return DbError::todo();
        }

        virtual DbError update(bool autoCommit) noexcept {
            return DbError::todo();
        }

        /** Fetch results */

        virtual int getColumnCount() const noexcept {
            return -1;
        }

        virtual DbError getColumnIndex(std::string_view name, int& index) const noexcept {
            int columnCount = getColumnCount();
            if (columnCount < 0)
                return DbError::todo();

            for (int i = 0; i < columnCount; ++i) {
                ColumnInfo info;
                if (DbError error = getColumnInfo(i, info))
                    return error;

                if (info.name == name) {
                    index = i;
                    return DbError::ok();
                }
            }

            return DbError::columnNotFound(name);
        }

        virtual DbError getColumnInfo(int index, ColumnInfo& info) const noexcept {
            return DbError::todo();
        }


        virtual DbError getIntByIndex(int index, int64& value) noexcept {
            return DbError::todo();
        }

        virtual DbError getBooleanByIndex(int index, bool& value) noexcept {
            return DbError::todo();
        }

        virtual DbError getStringByIndex(int index, std::string_view& value) noexcept {
            return DbError::todo();
        }

        virtual DbError getDoubleByIndex(int index, double& value) noexcept {
            return DbError::todo();
        }

        virtual DbError getBlobByIndex(int index, Blob& value) noexcept {
            return DbError::todo();
        }

        virtual DbError getIntByName(std::string_view column, int64& value) noexcept;
        virtual DbError getBooleanByName(std::string_view column, bool& value) noexcept;
        virtual DbError getStringByName(std::string_view column, std::string_view& value) noexcept;
        virtual DbError getDoubleByName(std::string_view column, double& value) noexcept;
        virtual DbError getBlobByName(std::string_view column, Blob& value) noexcept;


        /** Binding */

        virtual int getBindCount() const noexcept {
            return -1;
        }

        virtual DbError getBindIndex(std::string_view name, int& index) const noexcept {
            return DbError::todo();
        }

        virtual DbError getBindInfo(int index, BindInfo& info) const noexcept {
            return DbError::todo();
        }

        virtual DbError bindIntByIndex(int index, int64 value) noexcept {
            return DbError::todo();
        }

        virtual DbError bindBooleanByIndex(int index, bool value) noexcept {
            return DbError::todo();
        }

        virtual DbError bindStringByIndex(int index, std::string_view value) noexcept {
            return DbError::todo();
        }

        virtual DbError bindDoubleByIndex(int index, double value) noexcept {
            return DbError::todo();
        }

        virtual DbError bindBlobByIndex(int index, Blob value) noexcept {
            return DbError::todo();
        }

        virtual DbError bindNullByIndex(int index) noexcept {
            return DbError::todo();
        }

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

    DbError getSqliteEnv(IEnvironment **env, const EnvConfig& config) noexcept;
    DbError getPostgresEnv(IEnvironment **env) noexcept;
    DbError getMySqlEnv(IEnvironment **env) noexcept;
    DbError getOracleEnv(IEnvironment **env, const EnvConfig& config) noexcept;
    DbError getMsSqlEnv(IEnvironment **env) noexcept;
    DbError getDb2Env(IEnvironment **env) noexcept;
}
