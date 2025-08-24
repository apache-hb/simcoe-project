#include "stdafx.hpp"

#include "logs/logger.hpp"
#include "logs/appenders/channels.hpp"

namespace logs = sm::logs;
namespace appenders = sm::logs::appenders;

template<typename T>
void addChannel(T *channel) {
    std::unique_ptr<logs::ILogChannel> ptr{channel};
    logs::Logger::instance().addChannel(std::move(ptr));
}

void appenders::create(db::Connection connection) {
    addDatabaseChannel(std::move(connection));
}

bool appenders::addConsoleChannel() {
    if (!isConsoleAvailable())
        return false;

    addChannel(console());
    return true;
}

bool appenders::addDebugChannel() {
    return false;
#if 0
    if (!isDebugConsoleAvailable())
        return false;

    addChannel(debugConsole());
    return true;
#endif
}

bool appenders::addFileChannel(const fs::path& path) {
    addChannel(file(path));
    return true;
}

bool appenders::addDatabaseChannel(db::Connection connection) {
    std::unique_ptr<logs::IAsyncLogChannel> ptr{database(std::move(connection))};
    logs::Logger::instance().setAsyncChannel(std::move(ptr));
    return true;
}

void appenders::destroy(void) noexcept {
    logs::Logger::instance().destroy();
}
