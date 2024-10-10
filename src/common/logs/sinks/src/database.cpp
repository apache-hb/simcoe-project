#include "stdafx.hpp"

#include "logs/structured/logging.hpp"
#include "logs/structured/message.hpp"

#include "logs/structured/category.hpp"
#include "logs/structured/channel.hpp"
#include "logs/structured/channels.hpp"

#include "db/transaction.hpp"
#include "db/connection.hpp"

#include "logs.dao.hpp"

namespace db = sm::db;
namespace logs = sm::logs;
namespace structured = sm::logs::structured;

struct LogEntryPacket {
    uint64_t timestamp;
    const structured::MessageInfo *message;
    std::shared_ptr<structured::ArgStore> args;
};

static uint64_t getTimestamp() noexcept {
    auto now = std::chrono::system_clock::now();
    return uint64_t(now.time_since_epoch().count());
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

static std::atomic<bool> gHasErrors = false;

static void registerMessagesWithDb(
    db::Connection& connection,
    std::span<const structured::CategoryInfo> categories,
    std::span<const structured::MessageInfo> messages
) {
    connection.createTable(sm::dao::logs::LogSession::table());
    connection.createTable(sm::dao::logs::LogSeverity::table());
    connection.createTable(sm::dao::logs::LogCategory::table());
    connection.createTable(sm::dao::logs::LogMessage::table());
    connection.createTable(sm::dao::logs::LogMessageAttribute::table());
    connection.createTable(sm::dao::logs::LogEntry::table());
    connection.createTable(sm::dao::logs::LogEntryAttribute::table());

    db::Transaction tx(&connection);
    sm::dao::logs::LogSession session {
        .startTime = getTimestamp(),
    };

    auto insertSeverity = connection.prepareInsertOrUpdate<sm::dao::logs::LogSeverity>();
    auto insertCategory = connection.prepareInsertOrUpdate<sm::dao::logs::LogCategory>();
    auto insertMessage = connection.prepareInsertOrUpdate<sm::dao::logs::LogMessage>();
    auto insertAttribute = connection.prepareInsertOrUpdate<sm::dao::logs::LogMessageAttribute>();

    connection.insert(session);
    for (auto& severity : kLogSeverityOptions)
        insertSeverity.insert(severity);

    for (const structured::CategoryInfo& category : categories) {
        sm::dao::logs::LogCategory daoCategory {
            .hash = category.hash,
            .name = std::string{category.name},
        };

        insertCategory.insert(daoCategory);
    }

    for (const structured::MessageInfo& message : messages) {
        sm::dao::logs::LogMessage daoMessage {
            .hash = message.hash,
            .message = std::string{message.message},
            .severity = uint32_t(message.level),
            .category = message.category.hash,
            .path = std::string{message.location.file_name()},
            .line = message.location.line(),
            .function = std::string{message.location.function_name()},
        };

        insertMessage.insert(daoMessage);

        for (int i = 0; i < message.indexAttributeCount; i++) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
                .key = fmt::to_string(i),
                .messageHash = message.hash
            };

            insertAttribute.insert(daoAttribute);
        }

        for (const auto& attribute : message.namedAttributes) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
                .key = std::string{attribute.name},
                .messageHash = message.hash
            };

            insertAttribute.insert(daoAttribute);
        }
    }
}

class DbChannel final : public structured::ILogChannel {
    db::Connection mConnection;
    db::PreparedInsertReturning<sm::dao::logs::LogEntry> mInsertEntry = mConnection.prepareInsertReturningPrimaryKey<sm::dao::logs::LogEntry>();
    db::PreparedInsert<sm::dao::logs::LogEntryAttribute> mInsertAttribute = mConnection.prepareInsert<sm::dao::logs::LogEntryAttribute>();
    moodycamel::BlockingConcurrentQueue<LogEntryPacket> mQueue{1024};

    size_t mMaxPackets = 256;
    sm::UniquePtr<LogEntryPacket[]> mPacketBuffer = new LogEntryPacket[mMaxPackets];

    std::jthread mWorkerThread;

    void attach() override { }

    ~DbChannel() override {
        mWorkerThread.request_stop();

        // this message is after the request_stop to prevent deadlocks.
        // if the message queue is empty before entry to this function
        // the message thread will wait forever, this message ensures that there
        // is always at least 1 message in the queue when shutdown
        // is requested
        LOG_INFO(GlobalLog, "Cleaning up database structured logging thread");

        mWorkerThread.join();
    }

    void postMessage(structured::MessagePacket packet) noexcept override {
        // discard messages from the database channel to prevent infinite recursion
        if (structured::detail::gLogCategory<DbLog> == packet.message.category)
            return;

        mQueue.enqueue(LogEntryPacket {
            .timestamp = packet.timestamp,
            .message = &packet.message,
            .args = packet.args
        });
    }

    void commit(std::span<const LogEntryPacket> packets) noexcept {
        db::Transaction tx(&mConnection);

        for (const auto& [timestamp, message, args] : packets) {
            sm::dao::logs::LogEntry entry {
                .timestamp = timestamp,
                .messageHash = message->hash
            };

            auto id = mInsertEntry.tryInsert(entry);
            if (!id.has_value()) {
                gHasErrors = true;
                continue;
            }

            fmt::format_args params{*args};

            for (int i = 0; i < message->indexAttributeCount; i++) {
                auto arg = params.get(i);
                fmt::format_args args{&arg, 1};

                sm::dao::logs::LogEntryAttribute entryAttribute {
                    .entryId = id.value(),
                    .key = fmt::to_string(i),
                    .value = fmt::vformat("{}", args)
                };

                if (auto error = mInsertAttribute.tryInsert(entryAttribute)) {
                    gHasErrors = true;
                }
            }

            for (const auto& name : message->namedAttributes) {
                auto arg = params.get(fmt::string_view{name.name});
                fmt::format_args args{&arg, 1};

                sm::dao::logs::LogEntryAttribute entryAttribute {
                    .entryId = id.value(),
                    .key = fmt::to_string(name.name),
                    .value = fmt::vformat("{}", args)
                };

                if (auto error = mInsertAttribute.tryInsert(entryAttribute)) {
                    gHasErrors = true;
                }
            }
        }
    }

    void workerThread(const std::stop_token& stop) noexcept {
        while (!stop.stop_requested()) {
            size_t count = mQueue.wait_dequeue_bulk(mPacketBuffer.get(), mMaxPackets);
            commit(std::span(mPacketBuffer.get(), count));
        }

        // commit any remaining packets
        while (true) {
            size_t count = mQueue.try_dequeue_bulk(mPacketBuffer.get(), mMaxPackets);
            if (count == 0)
                break;

            commit(std::span(mPacketBuffer.get(), count));
        }
    }
public:
    DbChannel(db::Connection connection)
        : mConnection(std::move(connection))
        , mWorkerThread([this](const std::stop_token& stop) { workerThread(stop); })
    { }
};

structured::ILogChannel *sm::logs::structured::database(db::Connection connection) {
    auto [categories, messages] = getMessages();
    registerMessagesWithDb(connection, categories, messages);
    return new DbChannel(std::move(connection));
}
