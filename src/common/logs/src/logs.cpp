#include "logs/logs.hpp"

using namespace sm;
using namespace sm::logs;

void ILogger::log(Category category, Severity severity, std::string_view msg) {
    if (will_accept(severity)) {
        accept(Message{msg, category, severity, 0, 0});
    }
}
