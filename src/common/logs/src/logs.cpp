#include "logs/logs.hpp"

#include <chrono>

using namespace sm;
using namespace sm::logs;

void Logger::log(const Message &message) {
    if (!will_accept(message.severity)) return;

    for (ILogChannel* channel : mChannels) {
        channel->accept(message);
    }
}

void Logger::log(Category category, Severity severity, std::string_view msg) {
    // get current time in milliseconds
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    log(Message{msg, category, severity, uint32_t(timestamp), 0});
}

void Logger::add_channel(ILogChannel *channel) {
    mChannels.push_back(channel);
}

void Logger::remove_channel(ILogChannel *channel) {
    mChannels.erase(std::remove(mChannels.begin(), mChannels.end(), channel), mChannels.end());
}

void Logger::set_severity(Severity severity) {
    mSeverity = severity;
}

Severity Logger::get_severity() const {
    return mSeverity;
}

Logger& logs::get_logger() noexcept {
    static Logger logger{Severity::eInfo};
    return logger;
}

Sink logs::get_sink(Category category) noexcept {
    return Sink{get_logger(), category};
}
