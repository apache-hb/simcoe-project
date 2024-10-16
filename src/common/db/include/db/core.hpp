#pragma once

#include "core/core.hpp"
#include "core/memory/unique.hpp"

#include <expected>
#include <memory>
#include <string>
#include <chrono>

#include "logs/logging.hpp"

#include "core.meta.hpp"

LOG_MESSAGE_CATEGORY(DbLog, "DB");

namespace sm::db {
    class ResultSet;
    class QueryIterator;
    class Query;
    class PreparedStatement;
    class Connection;
    class Environment;

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
    enum class DbType : uint_least8_t {
#define DB_TYPE(id, enabled) id,
#include "db/orm.inc"

        eCount
    };

    REFLECT_ENUM(DataType)
    enum class DataType : uint_least8_t {
        eInteger, // int32_t
        eBoolean, // bool
        eString, // std::string
        eDouble, // double
        eBlob, // Blob
        eDateTime, // DateTime
        eNull, // ???
        eRowId, // ???

        eCount
    };

    REFLECT_ENUM(StatementType)
    enum class StatementType : uint_least8_t {
        eQuery, // select
        eModify, // update
        eDefine, // create
        eControl, // alter

        eCount
    };

    REFLECT_ENUM(JournalMode)
    enum class JournalMode : uint_least8_t {
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
    enum class Synchronous : uint_least8_t {
        eDefault,

        eExtra,
        eFull,
        eNormal,
        eOff,

        eCount
    };

    REFLECT_ENUM(LockingMode)
    enum class LockingMode : uint_least8_t {
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
        std::string connection;

        uint16 port;
        std::string host;
        std::string user;
        std::string password;
        std::string database;

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

template<>
struct fmt::formatter<sm::db::ConnectionConfig> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    constexpr auto format(const sm::db::ConnectionConfig& config, format_context& ctx) {
        return format_to(ctx.out(), "ConnectionConfig({}:{}, {}/{}, {}s)",
            config.host, config.port, config.user, config.database, config.timeout.count());
    }
};
