#include "stdafx.hpp"

#include "db/transaction.hpp"

#include "logs/structured/message.hpp"

#include "logs.dao.hpp"

namespace structured = sm::logs::structured;
namespace db = sm::db;

struct LogAttribute {
    uint64_t id;
    std::string value;
};

using LogMessageId = sm::logs::structured::detail::LogMessageId;
using MessageAttributeInfo = sm::logs::structured::MessageAttributeInfo;

using LogAttributeVec = sm::SmallVector<LogAttribute, structured::kMaxMessageAttributes>;
using AttributeInfoVec = sm::SmallVector<MessageAttributeInfo, structured::kMaxMessageAttributes>;

struct LogEntryPacket {
    uint64_t timestamp;
    uint64_t message;
    LogAttributeVec attributes;
};

LOG_CATEGORY_IMPL(gLog, "dblog");

static constexpr auto kLogSeverityOptions = std::to_array<sm::dao::logs::LogSeverity>({
    { "trace",   uint32_t(sm::logs::Severity::eTrace)   },
    { "debug",   uint32_t(sm::logs::Severity::eDebug)   },
    { "info",    uint32_t(sm::logs::Severity::eInfo)    },
    { "warning", uint32_t(sm::logs::Severity::eWarning) },
    { "error",   uint32_t(sm::logs::Severity::eError)   },
    { "fatal",   uint32_t(sm::logs::Severity::eFatal)   },
    { "panic",   uint32_t(sm::logs::Severity::ePanic)   }
});

static db::Connection *gConnection = nullptr;
static std::atomic<bool> gIsRunning = false;
static moodycamel::BlockingConcurrentQueue<LogEntryPacket> gLogQueue{1024};
static std::jthread *gLogThread = nullptr;

static void commitLogPackets(std::span<const LogEntryPacket> packets) noexcept {
    db::Transaction tx(gConnection);

    fmt::println(stderr, "Committing log packets {}", packets.size());

    for (const LogEntryPacket& packet : packets) {
        sm::dao::logs::LogEntry entry {
            .timestamp = packet.timestamp,
            .message = packet.message
        };

        auto id = gConnection->tryInsertReturningPrimaryKey(entry);
        if (!id.has_value()) {
            gLog.warn("Failed to insert log entry: {}", id.error().message());
            continue;
        }

        for (const auto& attribute : packet.attributes) {
            sm::dao::logs::LogEntryAttribute entryAttribute {
                .entry = id.value(),
                .key = attribute.id,
                .value = attribute.value
            };

            if (auto error = gConnection->tryInsert(entryAttribute)) {
                gLog.warn("Failed to insert log entry attribute: {}", error.message());
            }
        }
    }
}

static AttributeInfoVec getAttributes(std::string_view message) noexcept {
    AttributeInfoVec attributes;

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
    info.attributes = getAttributes(message.message);
    getLogMessages().emplace_back(*this);
}

void structured::detail::postLogMessage(const LogMessageId& message, sm::SmallVectorBase<std::string> args) noexcept {
    if (!gIsRunning) {
        gLog.warn("Log message posted after cleanup");
        return;
    }

    uint64_t timestamp = getTimestamp();

    ssize_t attrCount = message.info.attributes.ssize();
    CTASSERTF(args.ssize() == attrCount, "Incorrect number of message attributes (%zd != %zd)", args.size(), attrCount);

    LogAttributeVec attributes;
    for (ssize_t i = 0; i < attrCount; i++) {
        attributes.emplace_back(LogAttribute {
            .id = message.info.attributes[i].id,
            .value = std::move(args[i])
        });
    }

    gLogQueue.enqueue(LogEntryPacket {
        .timestamp = timestamp,
        .message = message.info.id,
        .attributes = std::move(attributes)
    });
}

db::DbError structured::setup(db::Connection& connection) {
    gConnection = &connection;

    connection.createTable(sm::dao::logs::LogSession::getTableInfo());
    connection.createTable(sm::dao::logs::LogSeverity::getTableInfo());
    connection.createTable(sm::dao::logs::LogMessage::getTableInfo());
    connection.createTable(sm::dao::logs::LogMessageAttribute::getTableInfo());
    connection.createTable(sm::dao::logs::LogEntry::getTableInfo());
    connection.createTable(sm::dao::logs::LogEntryAttribute::getTableInfo());

    sm::dao::logs::LogSession session {
        .startTime = getTimestamp(),
    };

    {
        db::Transaction tx(&connection);
        connection.insert(session);
        for (auto& severity : kLogSeverityOptions)
            connection.insertOrUpdate(severity);
    }

    for (LogMessageId& messageId : getLogMessages()) {
        LogMessageInfo& message = messageId.info;

        sm::dao::logs::LogMessage daoMessage {
            .message = std::string{message.message},
            .severity = uint32_t(message.level),
            .path = std::string{message.location.file_name()},
            .line = message.location.line(),
            .function = std::string{message.location.function_name()},
        };

        auto id = connection.insertReturningPrimaryKey(daoMessage);

        message.id = id;

        for (MessageAttributeInfo& attribute : messageId.info.attributes) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
                .key = std::string{attribute.name},
                .message = id
            };

            auto tagId = connection.insertReturningPrimaryKey(daoAttribute);

            attribute.id = tagId;
        }
    }

    // NOLINTNEXTLINE(performance-unnecessary-value-param) - std::stop_token should be passed by value
    gLogThread = new std::jthread([](std::stop_token stop) noexcept {
        LogEntryPacket packets[256];
        while (!stop.stop_requested()) {
            size_t count = gLogQueue.wait_dequeue_bulk(packets, std::size(packets));
            commitLogPackets(std::span(packets, count));
        }

        gIsRunning = false;

        // commit any remaining packets
        while (true) {
            size_t count = gLogQueue.try_dequeue_bulk(packets, std::size(packets));
            fmt::println(stderr, "Committing remaining log packets {}", count);
            if (count == 0)
                break;

            commitLogPackets(std::span(packets, count));
        }
    });

    gIsRunning = true;

    return db::DbError::ok();
}

void structured::cleanup() {
    gLogThread->request_stop();

    // this message is after the request_stop to prevent deadlocks.
    // if the message queue is empty before entry to this function
    // the message thread will wait forever, this message ensures that there
    // is always at least 1 message in the queue when shutdown
    // is requested
    LOG_INFO("Cleaning up structured logging");

    fmt::println(stderr, "Cleaning up structured logging");
    delete gLogThread;
    fmt::println(stderr, "Structured logging cleanup complete");
    gLogThread = nullptr;

    gConnection = nullptr;
}
