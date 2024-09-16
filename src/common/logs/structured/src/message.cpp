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
    uint64_t messageHash;
    LogAttributeVec attributes;
};

LOG_CATEGORY_IMPL(gFallbackLog, "Structed Logging");

static constexpr auto kLogSeverityOptions = std::to_array<sm::dao::logs::LogSeverity>({
    { "trace",   uint32_t(sm::logs::Severity::eTrace)   },
    { "debug",   uint32_t(sm::logs::Severity::eDebug)   },
    { "info",    uint32_t(sm::logs::Severity::eInfo)    },
    { "warning", uint32_t(sm::logs::Severity::eWarning) },
    { "error",   uint32_t(sm::logs::Severity::eError)   },
    { "fatal",   uint32_t(sm::logs::Severity::eFatal)   },
    { "panic",   uint32_t(sm::logs::Severity::ePanic)   }
});

static std::atomic<bool> gIsRunning = false;

class DbLogger {
    db::Connection& mConnection;
    db::PreparedInsertReturning<sm::dao::logs::LogEntry> mInsertEntry = mConnection.prepareInsertReturningPrimaryKey<sm::dao::logs::LogEntry>();
    db::PreparedInsert<sm::dao::logs::LogEntryAttribute> mInsertAttribute = mConnection.prepareInsert<sm::dao::logs::LogEntryAttribute>();
    moodycamel::BlockingConcurrentQueue<LogEntryPacket> mQueue;

    size_t mMaxPackets;
    sm::UniquePtr<LogEntryPacket[]> mPacketBuffer = new LogEntryPacket[mMaxPackets];

    std::jthread mWorkerThread;

    void commit(std::span<const LogEntryPacket> packets) noexcept {
        db::Transaction tx(&mConnection);

        for (const LogEntryPacket& packet : packets) {
            sm::dao::logs::LogEntry entry {
                .timestamp = packet.timestamp,
                .messageHash = packet.messageHash
            };

            auto id = mInsertEntry.tryInsert(entry);
            if (!id.has_value()) {
                gFallbackLog.warn("Failed to insert log entry: {}", id.error().message());
                continue;
            }

            for (size_t i = 0; i < packet.attributes.size(); i++) {
                sm::dao::logs::LogEntryAttribute entryAttribute {
                    .entryId = id.value(),
                    .param = uint32_t(i),
                    .value = packet.attributes[i].value
                };

                if (auto error = mInsertAttribute.tryInsert(entryAttribute)) {
                    gFallbackLog.warn("Failed to insert log entry attribute: {}", error.message());
                }
            }
        }
    }


public:
    DbLogger(db::Connection& connection) noexcept
        : mConnection(connection)
        , mQueue(1024)
        , mMaxPackets(256)
    { }

    void enqueue(LogEntryPacket&& packet) noexcept {
        mQueue.enqueue(std::move(packet));
    }

    void workerThread(std::stop_token stop) noexcept {
        while (!stop.stop_requested()) {
            size_t count = mQueue.wait_dequeue_bulk(mPacketBuffer.get(), mMaxPackets);
            commit(std::span(mPacketBuffer.get(), count));
        }

        gIsRunning = false;

        // commit any remaining packets
        while (true) {
            size_t count = mQueue.try_dequeue_bulk(mPacketBuffer.get(), mMaxPackets);
            fmt::println(stderr, "Committing remaining log packets {}", count);
            if (count == 0)
                break;

            commit(std::span(mPacketBuffer.get(), count));
        }
    }
};

static DbLogger *gLogger = nullptr;
static sm::UniquePtr<std::jthread> gLogThread = nullptr;

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
        gFallbackLog.warn("Log message posted after cleanup");
        return;
    }

    uint64_t timestamp = getTimestamp();

    ssize_t attrCount = message.info.attributes.ssize();
    CTASSERTF(args.ssize() == attrCount, "Incorrect number of message attributes (%zd != %zd)", args.ssize(), attrCount);

    LogAttributeVec attributes;
    for (ssize_t i = 0; i < attrCount; i++) {
        attributes.emplace_back(LogAttribute {
            .value = std::move(args[i])
        });
    }

    gLogger->enqueue(LogEntryPacket {
        .timestamp = timestamp,
        .messageHash = message.info.hash,
        .attributes = std::move(attributes)
    });
}

db::DbError structured::setup(db::Connection& connection) {
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

    for (const LogMessageId& messageId : getLogMessages()) {
        const LogMessageInfo& message = messageId.info;

        sm::dao::logs::LogMessage daoMessage {
            .hash = message.hash,
            .message = std::string{message.message},
            .severity = uint32_t(message.level),
            .path = std::string{message.location.file_name()},
            .line = message.location.line(),
            .function = std::string{message.location.function_name()},
        };

        connection.insertOrUpdate(daoMessage);

        for (size_t i = 0; i < message.attributes.size(); i++) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
                .key = std::string{message.attributes[i].name},
                .param = uint32_t(i),
                .messageHash = message.hash
            };

            connection.insertOrUpdate(daoAttribute);
        }
    }

    gLogger = new DbLogger(connection);
    gIsRunning = true;
    gLogThread = sm::makeUnique<std::jthread>([](std::stop_token stop) {
        gLogger->workerThread(stop);
    });

    return db::DbError::ok();
}

void structured::cleanup() {
    gIsRunning = false;
    gLogThread->request_stop();

    // this message is after the request_stop to prevent deadlocks.
    // if the message queue is empty before entry to this function
    // the message thread will wait forever, this message ensures that there
    // is always at least 1 message in the queue when shutdown
    // is requested
    LOG_INFO("Cleaning up structured logging");

    gFallbackLog.info("Cleaning up structured logging");
    gLogThread.reset();
    gFallbackLog.info("Structured logging cleaned up");

    delete gLogger;
    gLogger = nullptr;
}
