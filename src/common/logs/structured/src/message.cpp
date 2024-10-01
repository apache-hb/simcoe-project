#include "stdafx.hpp"

#include "db/transaction.hpp"

#include "logs/structured/channel.hpp"
#include "logs/structured/message.hpp"

#include "logs.dao.hpp"


namespace structured = sm::logs::structured;
namespace db = sm::db;

static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("just text").size() == 0);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("thing {0}", 1).size() == 1);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("multiple {0} second {0} identical {0} indices", 1).size() == 1);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("thing {0} second {1} third {name}", 1, 2, fmt::arg("name", "bob")).size() == 3);

struct LogAttribute {
    std::string value;
};

using Category = sm::logs::Category;
using LogMessageInfo = sm::logs::structured::LogMessageInfo;
using LogMessageId = sm::logs::structured::detail::LogMessageId;
using MessageAttributeInfo = sm::logs::structured::MessageAttributeInfo;

using LogAttributeVec = sm::SmallVector<LogAttribute, structured::kMaxMessageAttributes>;
using AttributeInfoVec = sm::SmallVector<MessageAttributeInfo, structured::kMaxMessageAttributes>;

struct LogEntryPacket {
    uint64_t timestamp;
    uint64_t messageHash;
    LogAttributeVec attributes;
    sm::logs::structured::detail::ArgStore args;
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
static std::atomic<bool> gHasErrors = false;

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
                fmt::println(stderr, "Failed to insert log entry {}", id.error().message());
                gFallbackLog.warn("Failed to insert log entry: {}", id.error().message());
                gHasErrors = true;
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
                    gHasErrors = true;
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

    void workerThread(const std::stop_token& stop) noexcept {
        while (!stop.stop_requested()) {
            size_t count = mQueue.wait_dequeue_bulk(mPacketBuffer.get(), mMaxPackets);
            commit(std::span(mPacketBuffer.get(), count));
        }

        gIsRunning = false;

        // commit any remaining packets
        while (true) {
            size_t count = mQueue.try_dequeue_bulk(mPacketBuffer.get(), mMaxPackets);
            if (count == 0)
                break;

            commit(std::span(mPacketBuffer.get(), count));
        }
    }
};

static sm::UniquePtr<DbLogger> gLogger;
static sm::UniquePtr<std::jthread> gLogThread;

static uint64_t getTimestamp() noexcept {
    auto now = std::chrono::system_clock::now();
    return uint64_t(now.time_since_epoch().count());
}

static std::vector<LogMessageInfo> &getLogMessages() noexcept {
    static std::vector<LogMessageInfo> sLogMessages;
    return sLogMessages;
}

static std::vector<Category> &getLogCategories() noexcept {
    static std::vector<Category> sLogCategories;
    return sLogCategories;
}

LogMessageId::LogMessageId(LogMessageInfo& message) noexcept
    : info(message)
{
    getLogMessages().emplace_back(message);
}

Category::Category(detail::LogCategoryData data) noexcept
    : data(data)
{
    getLogCategories().emplace_back(*this);
}

void structured::detail::postLogMessage(const LogMessageId& message, detail::ArgStore args) noexcept {
    if (!gIsRunning) {
        gFallbackLog.warn("Log message posted after cleanup");
        return;
    }

    uint64_t timestamp = getTimestamp();

    gLogger->enqueue(LogEntryPacket {
        .timestamp = timestamp,
        .messageHash = message.info.hash,
        .args = std::move(args)
    });
}

void structured::detail::postLogMessage(const LogMessageId& message, sm::SmallVectorBase<std::string> args) noexcept {
    if (!gIsRunning) {
        gFallbackLog.warn("Log message posted after cleanup");
        return;
    }

    uint64_t timestamp = getTimestamp();

    ssize_t attrCount = message.info.attributeCount();
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

bool structured::isRunning() noexcept {
    return !gHasErrors;
}

static void registerMessagesWithDb(db::Connection& connection) {
    connection.createTable(sm::dao::logs::LogSession::getTableInfo());
    connection.createTable(sm::dao::logs::LogSeverity::getTableInfo());
    connection.createTable(sm::dao::logs::LogCategory::getTableInfo());
    connection.createTable(sm::dao::logs::LogMessage::getTableInfo());
    connection.createTable(sm::dao::logs::LogMessageAttribute::getTableInfo());
    connection.createTable(sm::dao::logs::LogEntry::getTableInfo());
    connection.createTable(sm::dao::logs::LogEntryAttribute::getTableInfo());

    db::Transaction tx(&connection);
    sm::dao::logs::LogSession session {
        .startTime = getTimestamp(),
    };

    connection.insert(session);
    for (auto& severity : kLogSeverityOptions)
        connection.insertOrUpdate(severity);

    for (const Category& category : getLogCategories()) {
        sm::dao::logs::LogCategory daoCategory {
            .hash = category.hash(),
            .name = std::string{category.data.name},
        };

        connection.insertOrUpdate(daoCategory);
    }

    for (const LogMessageInfo& message : getLogMessages()) {
        sm::dao::logs::LogMessage daoMessage {
            .hash = message.hash,
            .message = std::string{message.message},
            .severity = uint32_t(message.level),
            .category = message.category.hash(),
            .path = std::string{message.location.file_name()},
            .line = message.location.line(),
            .function = std::string{message.location.function_name()},
        };

        connection.insertOrUpdate(daoMessage);

        for (int i = 0; i < message.indexAttributeCount; i++) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
                .key = fmt::to_string(i),
                .messageHash = message.hash
            };

            connection.insertOrUpdate(daoAttribute);
        }

        for (const auto& attribute : message.namedAttributes) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
                .key = std::string{attribute.name},
                .messageHash = message.hash
            };

            connection.insertOrUpdate(daoAttribute);
        }
    }
}

db::DbError structured::setup(db::Connection& connection) {
    registerMessagesWithDb(connection);

    gLogger = new DbLogger(connection);
    gIsRunning = true;
    gLogThread = sm::makeUnique<std::jthread>([](const std::stop_token& stop) {
        gLogger->workerThread(stop);
    });

    return db::DbError::ok();
}

void structured::cleanup() {
    gLogThread->request_stop();

    // this message is after the request_stop to prevent deadlocks.
    // if the message queue is empty before entry to this function
    // the message thread will wait forever, this message ensures that there
    // is always at least 1 message in the queue when shutdown
    // is requested
    LOG_INFO(GlobalLog, "Cleaning up structured logging");

    while (gIsRunning.load())
        std::this_thread::yield();

    gFallbackLog.info("Cleaning up structured logging");
    gLogThread.reset();
    gFallbackLog.info("Structured logging cleaned up");

    gLogger.reset();
}

void structured::Logger::addChannel(ILogChannel& channel) noexcept {
    channel.attach(getLogCategories(), getLogMessages());
    mChannels.emplace_back(&channel);
}

void structured::Logger::postMessage(const LogMessageInfo& message, const sm::SmallVectorBase<std::string>& args) noexcept {
    std::span<const std::string> span(args.data(), args.size());
    for (ILogChannel* channel : mChannels)
        channel->postMessage(message, span);
}

structured::Logger& structured::Logger::instance() noexcept {
    static Logger sInstance;
    return sInstance;
}
