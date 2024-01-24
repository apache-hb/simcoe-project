#include "logs/logs.hpp"

#include <chrono>

using namespace sm;
using namespace sm::logs;

void ILogger::log(Category category, Severity severity, std::string_view msg) {
    // get current time in milliseconds
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    if (will_accept(severity)) {
        accept(Message{msg, category, severity, uint32_t(ms), 0});
    }
}
