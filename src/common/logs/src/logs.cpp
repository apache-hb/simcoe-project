#include "stdafx.hpp"

#include "common.hpp"

using namespace sm;
using namespace sm::logs;

LOG_CATEGORY_IMPL(logs::gGlobal, "global");
LOG_CATEGORY_IMPL(logs::gDebug, "debug");
LOG_CATEGORY_IMPL(logs::gAssets, "assets");

struct TimeUnits {
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;
};

static constexpr TimeUnits getTimeUnits(uint32_t timestamp) {
    return TimeUnits {
        .hours = (timestamp / (60 * 60 * 1000)) % 24,
        .minutes = (timestamp / (60 * 1000)) % 60,
        .seconds = (timestamp / 1000) % 60,
        .milliseconds = timestamp % 1000,
    };
}

static constexpr colour_t getSeverityColour(Severity severity) {
    using enum logs::Severity;
    switch (severity) {
    case eTrace: return eColourWhite;
    case eInfo: return eColourGreen;
    case eWarning: return eColourYellow;
    case eError: return eColourRed;
    case ePanic: return eColourMagenta;
    default: return eColourDefault;
    }
}

static size_t buildMessageInner(sm::Span<char> buffer, const Message& message, const char *colour, const char *reset) {
    const auto& [msg, category, severity, timestamp, tid] = message;
    auto [h, m, s, ms] = getTimeUnits(timestamp);

    return fmt::format_to_n(buffer.data(), buffer.size(), "{}[{}]{}[{:02}:{:02}:{:02}.{:03}] {}:", colour, message.severity, reset, h, m, s, ms, message.category.name()).size;
}

size_t logs::buildMessageHeader(sm::Span<char> buffer, const Message& message) noexcept {
    return buildMessageInner(buffer, message, "", "");
}

size_t logs::buildMessageHeaderWithColour(sm::Span<char> buffer, const Message& message, const colour_pallete_t& pallete) noexcept {
    const char *colour = colour_get(&pallete, getSeverityColour(message.severity));
    const char *reset = colour_reset(&pallete);

    return buildMessageInner(buffer, message, colour, reset);
}

std::experimental::generator<sm::StringView> logs::splitMessage(sm::StringView message) {
    size_t start = 0;
    size_t end = 0;
    while (end < message.size()) {
        if (message[end] == '\n') {
            co_yield message.substr(start, end - start);
            start = end + 1;
        }
        end++;
    }

    if (start < end) {
        co_yield message.substr(start, end - start);
    }
}

void Logger::log(const Message &message) noexcept {
    if (!willAcceptMessage(message.severity)) return;

    for (ILogChannel* channel : mChannels) {
        channel->accept(message);
    }
}

void Logger::log(const LogCategory& category, Severity severity, sm::StringView msg) noexcept {
    // get current time in milliseconds
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    log(Message{msg, category, severity, uint32_t(timestamp), 0});
}

void Logger::addChannel(ILogChannel& channel) noexcept {
    mChannels.push_back(&channel);
}

void Logger::removeChannel(ILogChannel& channel) noexcept {
    mChannels.erase(std::remove(mChannels.begin(), mChannels.end(), &channel), mChannels.end());
}

void LogCategory::vlog(Severity severity, fmt::string_view format, fmt::format_args args) const noexcept {
    Logger& logger = getGlobalLogger();
    if (!logger.willAcceptMessage(severity)) return;

    auto text = fmt::vformat(format, args);
    logger.log(*this, severity, text);
}

Logger& logs::getGlobalLogger() noexcept {
    static Logger logger{Severity::eInfo};
    return logger;
}
