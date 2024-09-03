#include "stdafx.hpp"

#include "dao/utils.hpp"

#include "logs/structured/message.hpp"

#include "logs.dao.hpp"

namespace structured = sm::logs::structured;
namespace db = sm::db;

using LogMessageId = sm::logs::structured::detail::LogMessageId;
using MessageAttributeInfo = sm::logs::structured::MessageAttributeInfo;

struct BuiltLogMessage {
    std::span<const MessageAttributeInfo> attributes;
    sm::logs::Severity level;
    std::string_view message;
    std::source_location location;
    std::uint64_t hash;
};

static std::uint64_t hashLogMessage(std::string_view message) noexcept {
    return std::hash<std::string_view>{}(message);
}

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

        attributes.push_back({message.substr(start + 1, end - start - 1)});
        i = end + 1;
    }

    return attributes;
}

std::unordered_map<std::uint64_t, BuiltLogMessage> &getLogMessages() noexcept {
    static std::unordered_map<std::uint64_t, BuiltLogMessage> sLogMessages;
    return sLogMessages;
}

LogMessageId::LogMessageId(const LogMessageInfo& message) noexcept
    : hash(hashLogMessage(message.message))
    , attributes(getAttributes(message.message))
{
    getLogMessages().emplace(hash, BuiltLogMessage{attributes, message.level, message.message, message.location, hash});
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

db::DbError sm::logs::structured::setup(db::Connection& connection) noexcept {
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

    for (const auto& [hash, message] : getLogMessages()) {
        ::logs::dao::LogMessage daoMessage {
            .message = std::string{message.message},
            .level = uint32_t(message.level),
            .file = message.location.file_name(),
            .line = message.location.line(),
            .function = message.location.function_name()
        };

        auto id = TRY_UNWRAP(dao::insertGetPrimaryKey(connection, daoMessage));

        for (const auto& attribute : message.attributes) {
            ::logs::dao::LogMessageAttribute daoAttribute {
                .key = std::string{attribute.name},
                .type = "string",
                .message = id
            };

            if (auto error = dao::insert(connection, daoAttribute))
                return error;
        }
    }

    return db::DbError::ok();
}
