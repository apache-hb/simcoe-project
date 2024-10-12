#include "stdafx.hpp"

#include "logs/logger.hpp"
#include "logs/sinks/channels.hpp"

namespace logs = sm::logs;
namespace sinks = sm::logs::sinks;

template<typename T>
void addChannel(T *channel) {
    std::unique_ptr<logs::ILogChannel> ptr{channel};
    logs::Logger::instance().addChannel(std::move(ptr));
}

void sinks::create(db::Connection connection) {
    addDatabaseChannel(std::move(connection));
}

bool sinks::addConsoleChannel() {
    if (!isConsoleAvailable())
        return false;

    addChannel(console());
    return true;
}

bool sinks::addDebugChannel() {
    return false;
#if 0
    if (!isDebugConsoleAvailable())
        return false;

    addChannel(debugConsole());
    return true;
#endif
}

bool sinks::addFileChannel(const fs::path& path) {
    addChannel(file(path));
    return true;
}

bool sinks::addDatabaseChannel(db::Connection connection) {
    std::unique_ptr<logs::IAsyncLogChannel> ptr{database(std::move(connection))};
    logs::Logger::instance().setAsyncChannel(std::move(ptr));
    return true;
}

void sinks::destroy(void) noexcept {
    logs::Logger::instance().destroy();
}
