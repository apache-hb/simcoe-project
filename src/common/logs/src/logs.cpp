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
