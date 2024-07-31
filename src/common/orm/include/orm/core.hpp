#pragma once

#include "core/memory/unique.hpp"
#include "orm/error.hpp"

#include "core/core.hpp"
#include "core/macros.hpp"

#include <expected>
#include <span>

namespace sm::db {
    class ResultSet;
    class QueryIterator;
    class Query;
    class PreparedStatement;
    class Connection;
    class Environment;

    using Blob = std::span<const uint8>;
    using Date = std::chrono::time_point<std::chrono::system_clock>;

    namespace detail {
        struct IStatement;
        struct IConnection;
        struct IEnvironment;

        void destroyStatement(detail::IStatement *impl) noexcept;
        void destroyConnection(detail::IConnection *impl) noexcept;
        void destroyEnvironment(detail::IEnvironment *impl) noexcept;

        using StmtHandle = std::shared_ptr<IStatement>;
        using ConnectionHandle = FnUniquePtr<detail::IConnection, &destroyConnection>;
        using EnvHandle = FnUniquePtr<detail::IEnvironment, &destroyEnvironment>;
    }

    enum class DbType {
        eSqlite3,
        ePostgreSQL,
        eMySQL,
        eOracleDB,
        eMSSQL,
        eDB2
    };

    enum class DataType {
        eInteger,
        eBoolean,
        eString,
        eDouble,
        eBlob,
        eNull,
        eRowId
    };

    struct ConnectionConfig {
        uint16 port;
        std::string_view host;
        std::string_view user;
        std::string_view password;
        std::string_view database;

        int timeout = 5;

        bool autoCommit = true;
    };

    struct Column {
        std::string_view name;
        DataType type;
    };
}
