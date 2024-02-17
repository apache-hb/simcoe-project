#include "logs/logs.hpp"

#include <chrono>

using namespace sm;
using namespace sm::logs;

void ILogger::log(Category category, Severity severity, std::string_view msg) {
    // get current time in milliseconds
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    log(Message{msg, category, severity, uint32_t(timestamp), 0});
}

void ILogger::log(const Message& message) {
    if (will_accept(message.severity)) {
        accept(message);
    }
}

#if 0
#if SMC_ENABLE_LOGDB

static std::unordered_map<uint32_t, std::string> gMessages;

void logdb::add_message(const char *str, uint32_t hash) {
    if (auto it = gMessages.find(hash); it != gMessages.end()) {
        CTASSERTF(it->second == str, "hash collision between %s and %s", it->second.c_str(), str);
    }

    gMessages[hash] = str;
}

#endif
#endif
