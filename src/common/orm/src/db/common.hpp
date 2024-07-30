#pragma once

#include "orm/connection.hpp"

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

        /* misc */

        virtual DbError tableExists(std::string_view name, bool& exists) noexcept = 0;
    };

    struct IStatement {
        virtual ~IStatement() = default;

        /** Lifecycle */

        virtual DbError close() noexcept = 0;
        virtual DbError reset() noexcept = 0;

        /** Binding */

        virtual DbError bindInt(int index, int64 value) noexcept = 0;
        virtual DbError bindBoolean(int index, bool value) noexcept = 0;
        virtual DbError bindString(int index, std::string_view value) noexcept = 0;
        virtual DbError bindDouble(int index, double value) noexcept = 0;
        virtual DbError bindBlob(int index, Blob value) noexcept = 0;
        virtual DbError bindNull(int index) noexcept = 0;

        virtual DbError getBindIndex(std::string_view name, int& index) noexcept = 0;

        /** Execution */

        virtual DbError select() noexcept = 0;
        virtual DbError update(bool commit) noexcept = 0;

        /** Fetch results */

        virtual int columnCount() const noexcept = 0;
        virtual DbError next() noexcept = 0;

        virtual DbError getInt(int index, int64& value) noexcept = 0;
        virtual DbError getBoolean(int index, bool& value) noexcept = 0;
        virtual DbError getString(int index, std::string_view& value) noexcept = 0;
        virtual DbError getDouble(int index, double& value) noexcept = 0;
        virtual DbError getBlob(int index, Blob& value) noexcept = 0;
    };

    DbError sqlite(IEnvironment **env) noexcept;
    DbError postgres(IEnvironment **env) noexcept;
    DbError mysql(IEnvironment **env) noexcept;
    DbError oracledb(IEnvironment **env) noexcept;
    DbError mssql(IEnvironment **env) noexcept;
    DbError db2(IEnvironment **env) noexcept;
}
