#pragma once

#include "core/core.hpp"
#include "core/memory/unique.hpp"

#include <expected>
#include <span>
#include <chrono>

#include "core.meta.hpp"

namespace sm::db {
    class ResultSet;
    class QueryIterator;
    class Query;
    class PreparedStatement;
    class Connection;
    class Environment;

    using Blob = std::span<const uint8>;
    using DateTime = std::chrono::time_point<std::chrono::system_clock>;

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

    REFLECT_ENUM(DbType)
    enum class DbType {
#define DB_TYPE(id, str, enabled) id,
#include "db/orm.inc"

        eCount
    };

    REFLECT_ENUM(DataType)
    enum class DataType {
        eInteger, // int32_t
        eBoolean, // bool
        eString, // std::string
        eDouble, // double
        eBlob, // std::vector<std::byte>
        eNull, // ???
        eRowId, // ???

        eCount
    };

    REFLECT_ENUM(StatementType)
    enum class StatementType {
        eQuery, // select
        eModify, // update
        eDefine, // create
        eControl, // alter

        eCount
    };

    REFLECT_ENUM(JournalMode)
    enum class JournalMode {
        eDefault,

        eDelete,
        eTruncate,
        ePersist,
        eMemory,
        eWal,
        eOff,

        eCount
    };

    REFLECT_ENUM(Synchronous)
    enum class Synchronous {
        eDefault,

        eExtra,
        eFull,
        eNormal,
        eOff,

        eCount
    };

    REFLECT_ENUM(LockingMode)
    enum class LockingMode {
        eDefault,

        eRelaxed,
        eExclusive,

        eCount
    };

    REFLECT()
    struct Version {
        REFLECT_BODY(Version)

        std::string name; ///< user facing version string
        int major;
        int minor;
        int revision;
        int increment;
        int ext;
    };

    REFLECT()
    struct EnvConfig {
        REFLECT_BODY(EnvConfig)

        bool logQueries = false;
    };

    REFLECT()
    struct ConnectionConfig {
        REFLECT_BODY(ConnectionConfig)

        // connection string, if set will override other connection parameters
        std::string_view connection;

        uint16 port;
        std::string_view host;
        std::string_view user;
        std::string_view password;
        std::string_view database;

        std::chrono::seconds timeout{5};

        bool autoCommit = true;

        JournalMode journalMode = JournalMode::eDefault;
        Synchronous synchronous = Synchronous::eDefault;
        LockingMode lockingMode = LockingMode::eDefault;
    };

    struct ColumnInfo {
        std::string_view name;
        DataType type;
    };

    struct BindInfo {
        std::string_view name;
    };
}
