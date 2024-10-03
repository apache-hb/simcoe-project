#include "stdafx.hpp"

#include "logs/structured/channel.hpp"
#include "logs/structured/message.hpp"

namespace structured = sm::logs::structured;
namespace db = sm::db;

using CategoryInfo = sm::logs::structured::CategoryInfo;
using MessageInfo = sm::logs::structured::MessageInfo;

using CategoryId = sm::logs::structured::detail::CategoryId;
using MessageId = sm::logs::structured::detail::MessageId;

static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("just text").size() == 0);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("thing {0}", 1).size() == 1);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("multiple {0} second {0} identical {0} indices", 1).size() == 1);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("thing {0} second {1} third {name}", 1, 2, fmt::arg("name", "bob")).size() == 3);

// theres probably like 2 or 3 ticks of difference between these
// but it's not really a big deal
static const auto kStartTime = std::chrono::system_clock::now();
static const auto kStartTicks = std::chrono::high_resolution_clock::now();

static uint64_t getTimestamp() noexcept {
    // use kStartTime and kStartTicks to calculate current time in milliseconds
    // using high_resolution_clock is much faster than system_clock on windows
    auto now = std::chrono::high_resolution_clock::now();
    auto ticksSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(now - kStartTicks);
    auto timeSinceStart = kStartTime + ticksSinceStart;

    return std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceStart.time_since_epoch()).count();
}

static std::vector<MessageInfo> &getLogMessages() noexcept {
    static std::vector<MessageInfo> sLogMessages;
    return sLogMessages;
}

static std::vector<CategoryInfo> &getLogCategories() noexcept {
    static std::vector<CategoryInfo> sLogCategories;
    return sLogCategories;
}

MessageId::MessageId(const MessageInfo& message) noexcept
    : info(message)
{
    getLogMessages().emplace_back(message);
}

CategoryId::CategoryId(CategoryInfo data) noexcept
    : data(data)
{
    getLogCategories().emplace_back(this->data);
}

structured::MessageStore structured::getMessages() noexcept {
    structured::MessageStore result {
        .categories = getLogCategories(),
        .messages = getLogMessages(),
    };

    return result;
}

void structured::detail::postLogMessage(const MessageInfo& message, ArgStore args) noexcept {
    Logger::instance().postMessage(message, std::move(args));
}

void structured::setup(db::Connection& connection) {
    std::unique_ptr<ILogChannel> db{database(connection)};
    Logger::instance().addChannel(std::move(db));
}

void structured::cleanup() {
    Logger::instance().cleanup();
}

void structured::Logger::addChannel(std::unique_ptr<ILogChannel> channel) {
    channel->attach();
    mChannels.emplace_back(std::move(channel));
}

void structured::Logger::cleanup() noexcept {
    mChannels.clear();
}

void structured::Logger::postMessage(const MessageInfo& message, ArgStore args) noexcept {
    uint64_t timestamp = getTimestamp();
    std::shared_ptr<ArgStore> sharedArgs = std::make_shared<ArgStore>(std::move(args));

    for (auto& channel : mChannels) {
        LogMessagePacket packet = {
            .message = message,
            .timestamp = timestamp,
            .args = sharedArgs
        };
        channel->postMessage(packet);
    }
}

structured::Logger& structured::Logger::instance() noexcept {
    static Logger sInstance;
    return sInstance;
}
