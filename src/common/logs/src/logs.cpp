#include "logs/logs.hpp"

#include <chrono>

using namespace sm;
using namespace sm::logs;

LOG_CATEGORY_IMPL(logs::gGlobal, "global");
LOG_CATEGORY_IMPL(logs::gRender, "render");
LOG_CATEGORY_IMPL(logs::gSystem, "system");
LOG_CATEGORY_IMPL(logs::gInput, "input");
LOG_CATEGORY_IMPL(logs::gAudio, "audio");
LOG_CATEGORY_IMPL(logs::gNetwork, "network");
LOG_CATEGORY_IMPL(logs::gService, "service");
LOG_CATEGORY_IMPL(logs::gPhysics, "physics");
LOG_CATEGORY_IMPL(logs::gUi, "ui");
LOG_CATEGORY_IMPL(logs::gDebug, "debug");
LOG_CATEGORY_IMPL(logs::gGpuApi, "rhi");
LOG_CATEGORY_IMPL(logs::gAssets, "assets");

void Logger::log(const Message &message) {
    if (!will_accept(message.severity)) return;

    for (ILogChannel* channel : mChannels) {
        channel->accept(message);
    }
}

void Logger::log(const LogCategory& category, Severity severity, sm::StringView msg) {
    // get current time in milliseconds
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    log(Message{msg, category, severity, uint32_t(timestamp), 0});
}

void Logger::log(Category category, Severity severity, sm::StringView msg) {
    using Reflect = ctu::TypeInfo<Category>;
    LogCategory cat{Reflect::to_string(category).c_str()};
    log(cat, severity, msg);
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

void LogCategory::vlog(Severity severity, fmt::string_view format, fmt::format_args args) const {
    Logger& logger = get_logger();
    if (!logger.will_accept(severity)) return;

    auto text = fmt::vformat(format, args);
    logger.log(*this, severity, text);
}

Logger& logs::get_logger() noexcept {
    static Logger logger{Severity::eInfo};
    return logger;
}
