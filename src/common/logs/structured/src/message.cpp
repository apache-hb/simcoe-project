#include "orm/transaction.hpp"
#include "stdafx.hpp"

#include "dao/utils.hpp"

#include "logs/structured/message.hpp"

#include "logs.dao.hpp"

namespace structured = sm::logs::structured;
namespace db = sm::db;

using LogMessageId = sm::logs::structured::detail::LogMessageId;
using MessageAttributeInfo = sm::logs::structured::MessageAttributeInfo;

struct BuiltMessageAttribute {
    uint64_t id;
};

struct BuiltLogMessage {
    std::span<const BuiltMessageAttribute> attributes;
    sm::logs::Severity level;
    std::string_view message;
    uint32_t line;
    std::string_view file;
    std::string_view function;
};

static db::Connection *gConnection = nullptr;

static std::vector<structured::MessageAttributeInfo> getAttributes(std::string_view message) noexcept {
    std::vector<structured::MessageAttributeInfo> attributes;

    size_t i = 0;
    while (i < message.size()) {
        auto start = message.find_first_of('{', i);
        if (start == std::string_view::npos) {
            break;
        }

        auto end = message.find_first_of('}', start);
        if (end == std::string_view::npos) {
            break;
        }

        MessageAttributeInfo info {
            .id = UINT64_MAX,
            .name = message.substr(start + 1, end - start - 1)
        };

        attributes.emplace_back(info);
        i = end + 1;
    }

    return attributes;
}

std::vector<LogMessageId> &getLogMessages() noexcept {
    static std::vector<LogMessageId> sLogMessages;
    return sLogMessages;
}

LogMessageId::LogMessageId(LogMessageInfo& message) noexcept
    : info(message)
    , attributes(getAttributes(message.message))
{
    getLogMessages().emplace_back(*this);
}

void structured::detail::postLogMessage(const LogMessageId& message, fmt::format_args args) noexcept {
    ::logs::dao::LogEntry entry {
        .timestamp = uint64_t(std::chrono::system_clock::now().time_since_epoch().count()),
        .message = message.info.id,
    };

    fmt::println(stderr, "inserting entry: id `{}`", message.info.id);
    auto id = dao::insertGetPrimaryKey(*gConnection, entry);
    if (!id.has_value()) {
        fmt::println(stderr, "Failed to insert log entry: {}", id.error().message());
        return;
    }

    for (const auto& attribute : message.attributes) {
        ::logs::dao::LogEntryAttribute entryAttribute {
            .entry = id.value(),
            .key = attribute.id,
            .value = "TODO",
        };


        if (auto error = dao::insert(*gConnection, entryAttribute))
            fmt::println(stderr, "Failed to insert log entry attribute: {}", error.message());
    }
}

static constexpr auto kLogSeverityOptions = std::to_array<::logs::dao::LogSeverity>({
    { "trace",   uint32_t(sm::logs::Severity::eTrace)   },
    { "debug",   uint32_t(sm::logs::Severity::eDebug)   },
    { "info",    uint32_t(sm::logs::Severity::eInfo)    },
    { "warning", uint32_t(sm::logs::Severity::eWarning) },
    { "error",   uint32_t(sm::logs::Severity::eError)   },
    { "fatal",   uint32_t(sm::logs::Severity::eFatal)   },
    { "panic",   uint32_t(sm::logs::Severity::ePanic)   }
});

db::DbError sm::logs::structured::setup(db::Connection& connection) {
    gConnection = &connection;

    if (auto error = dao::createTable<::logs::dao::LogSeverity>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    for (const auto& severity : kLogSeverityOptions)
        if (auto error = dao::insert(connection, severity))
            return error;

    if (auto error = dao::createTable<::logs::dao::LogMessage>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    if (auto error = dao::createTable<::logs::dao::LogMessageAttribute>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    if (auto error = dao::createTable<::logs::dao::LogEntry>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    if (auto error = dao::createTable<::logs::dao::LogEntryAttribute>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    for (auto& messageId : getLogMessages()) {
        auto& message = messageId.info;

        ::logs::dao::LogMessage daoMessage {
            .message = std::string{message.message},
            .level = uint32_t(message.level),
            .file = std::string{message.file},
            .line = message.line,
            .function = std::string{message.function},
        };

        auto id = TRY_UNWRAP(dao::insertGetPrimaryKey(connection, daoMessage));

        message.id = id;

        for (auto& attribute : messageId.attributes) {
            ::logs::dao::LogMessageAttribute daoAttribute {
                .key = std::string{attribute.name},
                .type = "string",
                .message = id
            };

            auto tagId = TRY_UNWRAP(dao::insertGetPrimaryKey(connection, daoAttribute));

            attribute.id = tagId;
        }
    }

    return db::DbError::ok();
}
