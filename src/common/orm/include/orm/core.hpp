#pragma once

#include "core/memory/unique.hpp"

#include "core/core.hpp"

#include <expected>
#include <span>
#include <chrono>

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
#define DB_TYPE(id, str, enabled) id,
#include "orm/orm.inc"
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

    enum class StatementType {
        eQuery,
        eModify,
        eDefine,
        eControl
    };

    struct Version {
        std::string name;
        int major;
        int minor;
        int patch;
    };

    struct ConnectionConfig {
        // connection string, if set will override other connection parameters
        std::string_view connection;

        uint16 port;
        std::string_view host;
        std::string_view user;
        std::string_view password;
        std::string_view database;

        std::chrono::seconds timeout{5};

        bool autoCommit = true;
    };

    struct ColumnInfo {
        std::string_view name;
        DataType type;
    };

    struct BindInfo {
        std::string_view name;
    };
}
