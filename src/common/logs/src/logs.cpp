#include "logs/logs.hpp"

#include <chrono>

using namespace sm;
using namespace sm::logs;

void ILogger::log(Category category, Severity severity, std::string_view msg, uint32_t timestamp) {
    // get current time in milliseconds
    if (timestamp == 0) {
        auto now = std::chrono::system_clock::now();
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }

    if (will_accept(severity)) {
        accept(Message{msg, category, severity, timestamp, 0});
    }
}
