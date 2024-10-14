#include "stdafx.hpp"

#include "logs/logger.hpp"
#include "logs/logging.hpp"
#include "logs/message.hpp"

#include "logs/category.hpp"
#include "logs/channel.hpp"
#include "logs/sinks/channels.hpp"

#include "db/transaction.hpp"
#include "db/connection.hpp"

#include "logs.dao.hpp"

namespace db = sm::db;
namespace logs = sm::logs;
namespace sinks = sm::logs::sinks;

struct LogEntryPacket {
    uint64_t timestamp;
    const logs::MessageInfo *message;
    std::unique_ptr<logs::DynamicArgStore> args;
};

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
    std::span<const logs::CategoryInfo> categories,
    std::span<const logs::MessageInfo> messages
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
        .startTime = logs::getCurrentTime()
    };

    auto insertSeverity = connection.prepareInsertOrUpdate<sm::dao::logs::LogSeverity>();
    auto insertCategory = connection.prepareInsertOrUpdate<sm::dao::logs::LogCategory>();
    auto insertMessage = connection.prepareInsertOrUpdate<sm::dao::logs::LogMessage>();
    auto insertAttribute = connection.prepareInsertOrUpdate<sm::dao::logs::LogMessageAttribute>();

    connection.insert(session);
    for (auto& severity : kLogSeverityOptions)
        insertSeverity.insert(severity);

    for (const logs::CategoryInfo& category : categories) {
        sm::dao::logs::LogCategory daoCategory {
            .hash = category.hash,
            .name = std::string{category.name},
        };

        insertCategory.insert(daoCategory);
    }

    for (const logs::MessageInfo& message : messages) {
        sm::dao::logs::LogMessage daoMessage {
            .hash = message.getHash(),
#if SM_LOGS_INCLUDE_INFO
            .message = std::string{message.getMessage()},
#endif
            .severity = uint32_t(message.getSeverity()),
            .category = message.getCategory().hash,
#if SM_LOGS_INCLUDE_INFO
            .path = std::string{message.getFileName()},
            .line = message.getLine(),
            .function = std::string{message.getFunction()},
#endif
        };

        insertMessage.insert(daoMessage);

        for (int i = 0; i < message.indexAttributeCount; i++) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
                .key = fmt::to_string(i),
                .messageHash = message.getHash()
            };

            insertAttribute.insert(daoAttribute);
        }

        for (const auto& attribute : message.namedAttributes) {
            sm::dao::logs::LogMessageAttribute daoAttribute {
                .key = std::string{attribute.name},
                .messageHash = message.getHash()
            };

            insertAttribute.insert(daoAttribute);
        }
    }
}

class DbChannel final : public logs::IAsyncLogChannel {
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

    void postMessageAsync(logs::AsyncMessagePacket packet) noexcept override {
        // discard messages from the database channel to prevent infinite recursion
        if (logs::detail::gLogCategory<DbLog> == packet.message.getCategory())
            return;

        mQueue.enqueue(LogEntryPacket {
            .timestamp = packet.timestamp,
            .message = &packet.message,
            .args = std::move(packet.args)
        });
    }

    void commit(std::span<const LogEntryPacket> packets) noexcept {
        db::Transaction tx(&mConnection);

        for (const auto& [timestamp, message, params] : packets) {
            sm::dao::logs::LogEntry entry {
                .timestamp = timestamp,
                .messageHash = message->getHash()
            };

            auto id = mInsertEntry.tryInsert(entry);
            if (!id.has_value()) {
                gHasErrors = true;
                continue;
            }

            for (int i = 0; i < message->indexAttributeCount; i++) {
                auto arg = params->get(i);
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

            for (const auto& [name] : message->namedAttributes) {
                auto arg = params->get(name);
                fmt::format_args args{&arg, 1};

                sm::dao::logs::LogEntryAttribute entryAttribute {
                    .entryId = id.value(),
                    .key = fmt::to_string(name),
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

logs::IAsyncLogChannel *sm::logs::sinks::database(db::Connection connection) {
    auto [categories, messages] = getMessages();
    registerMessagesWithDb(connection, categories, messages);
    return new DbChannel(std::move(connection));
}
