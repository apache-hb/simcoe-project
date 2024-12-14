#pragma once

#include <simcoe_db_config.h>

#include "core/core.hpp"
#include "core/memory/unique.hpp"

#include <expected>
#include <memory>
#include <string>
#include <chrono>

#include "logs/logging.hpp"

LOG_MESSAGE_CATEGORY(DbLog, "DB");

namespace sm::db {
    class ResultSet;
    class PreparedStatement;
    class Connection;
    class Environment;

    namespace detail {
        struct IStatement;
        class IConnection;
        struct IEnvironment;

        void destroyStatement(detail::IStatement *impl) noexcept;
        void destroyConnection(detail::IConnection *impl) noexcept;
        void destroyEnvironment(detail::IEnvironment *impl) noexcept;

        using StmtHandle = std::shared_ptr<IStatement>;
        using ConnectionHandle = FnUniquePtr<detail::IConnection, &destroyConnection>;
        using EnvHandle = FnUniquePtr<detail::IEnvironment, &destroyEnvironment>;
    }

    enum class DbType : uint_least8_t {
#define DB_TYPE(id, name, enabled) id,
#include "db/db.inc"

        eCount
    };

    enum class DataType : uint_least8_t {
#define DB_DATATYPE(id, name) id,
#include "db/db.inc"

        eCount
    };

    constexpr bool isStringDataType(DataType type) noexcept {
        return type == DataType::eString
            || type == DataType::eChar
            || type == DataType::eVarChar;
    }

    enum class StatementType : uint_least8_t {
        eQuery, // select
        eModify, // update
        eDefine, // create
        eControl, // alter

        eCount
    };

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

    enum class Synchronous : uint_least8_t {
        eDefault,

        eExtra,
        eFull,
        eNormal,
        eOff,

        eCount
    };

    enum class LockingMode : uint_least8_t {
        eDefault,

        eRelaxed,
        eExclusive,

        eCount
    };

    enum class AssumeRole : uint_least8_t {
        eDefault,

        eSYSDBA,
        eSYSOPER,
        eSYSASM,
        eSYSBKP,
        eSYSDGD,
        eSYSKMT,
        eSYSRAC,

        eCount
    };

    enum class Permission {
        eNone = 0,

        eRead = (1 << 0),
        eWrite = (1 << 1),

        eAll = eRead | eWrite
    };

    std::string_view toString(DbType type) noexcept;
    std::string_view toString(DataType type) noexcept;
    std::string_view toString(StatementType type) noexcept;
    std::string_view toString(JournalMode mode) noexcept;
    std::string_view toString(Synchronous mode) noexcept;
    std::string_view toString(LockingMode mode) noexcept;
    std::string_view toString(AssumeRole role) noexcept;

    struct Version {
        std::string name; ///< user facing version string
        int major;
        int minor;
        int revision;
        int increment;
        int ext;

        bool isKnown() const noexcept {
            return major != 0 || minor != 0 || revision != 0 || increment != 0 || ext != 0;
        }
    };

    struct EnvConfig {
        bool logQueries = false;
    };

    static constexpr std::chrono::seconds kDefaultTimeout{5};

    /// @brief DB connection configuration
    ///
    /// Used to connect to a database.
    /// Some fields may be ignored depending on the driver.
    /// @a ConnectionConfig::connection will always be used if set.
    /// @a ConnectionConfig::alias will be used if set and @a ConnectionConfig::connection is not set.
    struct ConnectionConfig {
        /// @brief Connection string
        /// @note if set will override other connection parameters
        std::string connection;

        /// @brief ODBC alias
        /// @warning Only used if @a connection is not set
        std::string alias;

        uint16_t port = 0;
        std::string host;
        std::string user;
        std::string password;
        std::string database;

        /// @brief Connection timeout
        /// How long to wait for a connection to be established
        std::chrono::seconds timeout = kDefaultTimeout;

        bool autoCommit = true;

        /// @brief Journal mode
        /// @note Only supported by SQLite
        JournalMode journalMode = JournalMode::eDefault;

        /// @brief Synchronous mode
        /// @note Only supported by SQLite
        Synchronous synchronous = Synchronous::eDefault;

        /// @brief Locking mode
        /// @note Only supported by SQLite
        LockingMode lockingMode = LockingMode::eDefault;

        /// @brief Connect as an operator user
        /// @note Only supported on OracleDB
        AssumeRole role = AssumeRole::eDefault;
    };

    std::string toString(const ConnectionConfig& config);

    struct ColumnInfo {
        std::string_view name;
        DataType type;
    };

    struct BindInfo {
        std::string_view name;
    };
}
