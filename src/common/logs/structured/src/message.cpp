#include "stdafx.hpp"

#include "dao/utils.hpp"

#include "logs/structured/message.hpp"

#include "logs.dao.hpp"

namespace structured = sm::logs::structured;
namespace db = sm::db;

using LogMessageId = sm::logs::structured::detail::LogMessageId;
using MessageAttributeInfo = sm::logs::structured::MessageAttributeInfo;

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
            .id = INT64_MAX,
            .name = message.substr(start + 1, end - start - 1)
        };

        attributes.emplace_back(info);
        i = end + 1;
    }

    return attributes;
}

static uint64_t getTimestamp() noexcept {
    auto now = std::chrono::system_clock::now();
    return uint64_t(now.time_since_epoch().count());
}

static std::vector<LogMessageId> &getLogMessages() noexcept {
    static std::vector<LogMessageId> sLogMessages;
    return sLogMessages;
}

LogMessageId::LogMessageId(LogMessageInfo& message) noexcept
    : info(message)
{
    message.attributes = getAttributes(message.message);
    getLogMessages().emplace_back(*this);
}

void structured::detail::postLogMessage(const LogMessageId& message, fmt::format_args args) noexcept {
    sm::dao::logs::LogEntry entry {
        .timestamp = getTimestamp(),
        .message = message.info.id,
    };

    fmt::println(stderr, "inserting entry: id `{}`", message.info.id);
    auto id = dao::insertGetPrimaryKey(*gConnection, entry);
    if (!id.has_value()) {
        fmt::println(stderr, "Failed to insert log entry: {}", id.error().message());
        return;
    }

    for (const auto& attribute : message.info.attributes) {
        fmt::println(stderr, "inserting attribute: id `{}` {}", attribute.id, (void*)&attribute);
        sm::dao::logs::LogEntryAttribute entryAttribute {
            .entry = id.value(),
            .key = attribute.id,
            .value = "TODO",
        };

        if (auto error = dao::insert(*gConnection, entryAttribute))
            fmt::println(stderr, "Failed to insert log entry attribute: {}", error.message());
    }
}

static constexpr auto kLogSeverityOptions = std::to_array<sm::dao::logs::LogSeverity>({
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

    if (auto error = dao::createTable<sm::dao::logs::LogSession>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    sm::dao::logs::LogSession session {
        .start = getTimestamp(),
    };

    if (auto error = dao::insert(connection, session))
        return error;

    if (auto error = dao::createTable<sm::dao::logs::LogSeverity>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    for (const auto& severity : kLogSeverityOptions)
        if (auto error = dao::insert(connection, severity))
            return error;

    if (auto error = dao::createTable<sm::dao::logs::LogMessage>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    if (auto error = dao::createTable<sm::dao::logs::LogMessageAttribute>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    if (auto error = dao::createTable<sm::dao::logs::LogEntry>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    if (auto error = dao::createTable<sm::dao::logs::LogEntryAttribute>(connection, dao::CreateTable::eCreateIfNotExists))
        return error;

    for (auto& messageId : getLogMessages()) {
        auto& message = messageId.info;

        sm::dao::logs::LogMessage daoMessage {
            .message = std::string{message.message},
            .level = uint32_t(message.level),
            .file = std::string{message.location.file_name()},
            .line = message.location.line(),
            .function = std::string{message.location.function_name()},
        };

        auto id = TRY_UNWRAP(dao::insertGetPrimaryKey(connection, daoMessage));

        message.id = id;

        for (auto& attribute : messageId.info.attributes) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
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
