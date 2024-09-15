#include "stdafx.hpp"

#include "db/transaction.hpp"

#include "logs/structured/message.hpp"

#include "logs.dao.hpp"

namespace structured = sm::logs::structured;
namespace db = sm::db;

struct LogAttribute {
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

static constexpr size_t kBufferCount = 512;

#if SMC_LOG_MESSAGE_TEXT
static constexpr auto kLogSeverityOptions = std::to_array<sm::dao::logs::LogSeverity>({
    { "trace",   uint32_t(sm::logs::Severity::eTrace)   },
    { "debug",   uint32_t(sm::logs::Severity::eDebug)   },
    { "info",    uint32_t(sm::logs::Severity::eInfo)    },
    { "warning", uint32_t(sm::logs::Severity::eWarning) },
    { "error",   uint32_t(sm::logs::Severity::eError)   },
    { "fatal",   uint32_t(sm::logs::Severity::eFatal)   },
    { "panic",   uint32_t(sm::logs::Severity::ePanic)   }
});
#endif

// NOLINTBEGIN(performance-unnecessary-value-param) - std::stop_token should be passed by value

class LogState {
    db::Connection& mConnection;
    db::PreparedInsertReturning<sm::dao::logs::LogEntry> mInsertEntry;
    db::PreparedInsert<sm::dao::logs::LogEntryAttribute> mInsertAttribute;
    moodycamel::BlockingConcurrentQueue<LogEntryPacket> mQueue;

    void commitPackets(std::span<const LogEntryPacket> packets) noexcept {
        db::Transaction tx(&mConnection);

        for (const LogEntryPacket& packet : packets) {
            sm::dao::logs::LogEntry entry {
                .timestamp = packet.timestamp,
                .message = packet.message
            };

            auto id = mInsertEntry.tryInsert(entry);
            if (!id.has_value()) {
                gLog.warn("Failed to insert log entry: {}", id.error().message());
                continue;
            }

            for (size_t i = 0; i < packet.attributes.size(); i++) {
                sm::dao::logs::LogEntryAttribute entryAttribute {
                    .entry = id.value(),
                    .param = i,
                    .value = packet.attributes[i].value
                };

                if (auto error = mInsertAttribute.tryInsert(entryAttribute)) {
                    gLog.warn("Failed to insert log entry attribute: {}", error.message());
                }
            }
        }
    }

public:
    LogState(db::Connection& db, size_t queueSize)
        : mConnection(db)
        , mInsertEntry(db.prepareInsertReturningPrimaryKey<sm::dao::logs::LogEntry>())
        , mInsertAttribute(db.prepareInsert<sm::dao::logs::LogEntryAttribute>())
        , mQueue(queueSize)
    { }

    void dequeuePackets(std::stop_token stop, std::span<LogEntryPacket> storage) noexcept {
        while (!stop.stop_requested()) {
            size_t count = mQueue.wait_dequeue_bulk(storage.data(), storage.size());
            commitPackets(std::span(storage.data(), count));
        }
    }

    void flushPackets(std::span<LogEntryPacket> storage) noexcept {
        while (true) {
            size_t count = mQueue.try_dequeue_bulk(storage.data(), storage.size());
            if (count == 0)
                break;

            commitPackets(std::span(storage.data(), count));
        }
    }

    void enqueue(LogEntryPacket&& packet) noexcept {
        mQueue.enqueue(std::move(packet));
    }
};

static std::atomic<bool> gIsRunning = false;
static sm::UniquePtr<LogState> gState = nullptr;
static sm::UniquePtr<std::jthread> gLogThread = nullptr;

static uint64_t getTimestamp() noexcept {
    auto now = std::chrono::system_clock::now();
    return uint64_t(now.time_since_epoch().count());
}

static std::vector<LogMessageId> &getLogMessages() noexcept {
    static std::vector<LogMessageId> sLogMessages;
    return sLogMessages;
}

LogMessageId::LogMessageId(const LogMessageInfo& message) noexcept
    : info(message)
{
    getLogMessages().emplace_back(*this);
}

void structured::detail::postLogMessage(const LogMessageId& message, sm::SmallVectorBase<std::string> args) noexcept {
    if (!gIsRunning) {
        gLog.warn("Log message posted after cleanup");
        return;
    }

    uint64_t timestamp = getTimestamp();

    ssize_t attrCount = message.info.getAttributeCount();
    CTASSERTF(args.ssize() == attrCount, "Incorrect number of message attributes (%zd != %zd)", args.size(), attrCount);

    LogAttributeVec attributes;
    for (ssize_t i = 0; i < attrCount; i++) {
        attributes.emplace_back(LogAttribute {
            .value = std::move(args[i])
        });
    }

    gState->enqueue(LogEntryPacket {
        .timestamp = timestamp,
        .message = message.info.hash,
        .attributes = std::move(attributes)
    });
}

db::DbError structured::setup(db::Connection& connection) {
    connection.createTable(sm::dao::logs::LogSession::getTableInfo());

#if SMC_LOG_MESSAGE_TEXT
    connection.createTable(sm::dao::logs::LogSeverity::getTableInfo());
#endif

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

#if SMC_LOG_MESSAGE_TEXT
        for (auto& severity : kLogSeverityOptions)
            connection.insertOrUpdate(severity);
#endif
    }

    for (const LogMessageId& messageId : getLogMessages()) {
        const LogMessageInfo& message = messageId.info;

        sm::dao::logs::LogMessage daoMessage {
            .hash = message.hash,

#if SMC_LOG_MESSAGE_TEXT
            .message = std::string{message.message},
            .severity = uint32_t(message.level),
            .path = std::string{message.location.file_name()},
            .line = message.location.line(),
            .function = std::string{message.location.function_name()},
#endif
        };

        connection.insertOrUpdate(daoMessage);

        for (size_t i = 0; i < message.getAttributeCount(); i++) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
#if SMC_LOG_MESSAGE_TEXT
                .key = std::string{messageId.info.attributes[i].name},
#endif

                .param = uint32_t(i),
                .message = message.hash
            };

            // TODO: should have a connection method to insert or update generating a primary key
            connection.insertOrUpdate(daoAttribute);
        }
    }

    gState = new LogState(connection, 0x1000);

    gLogThread = new std::jthread([](std::stop_token stop) noexcept {
        sm::UniquePtr buffer = sm::makeUnique<LogEntryPacket[]>(kBufferCount);
        gState->dequeuePackets(stop, std::span(buffer.get(), kBufferCount));

        gIsRunning = false;
        gState->flushPackets(std::span(buffer.get(), kBufferCount));
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

    gLogThread.reset();
    gState.reset();
}

// NOLINTEND(performance-unnecessary-value-param)
