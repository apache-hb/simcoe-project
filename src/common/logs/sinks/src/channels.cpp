#include "stdafx.hpp"

#include "logs/structured/logger.hpp"
#include "logs/structured/channels.hpp"

namespace structured = sm::logs::structured;

template<typename T>
void addChannel(T *channel) {
    std::unique_ptr<structured::ILogChannel> ptr{channel};
    structured::Logger::instance().addChannel(std::move(ptr));
}

void structured::create(db::Connection connection) {
    addDatabaseChannel(std::move(connection));
}

bool structured::addConsoleChannel() {
    if (!isConsoleAvailable())
        return false;

    addChannel(console());
    return true;
}

bool structured::addDebugChannel() {
    if (!isDebugConsoleAvailable())
        return false;

    addChannel(debugConsole());
    return true;
}

bool structured::addFileChannel(const fs::path& path) {
    addChannel(file(path));
    return true;
}

bool structured::addDatabaseChannel(db::Connection connection) {
    addChannel(database(std::move(connection)));
    return true;
}

void structured::destroy(void) noexcept {
    structured::Logger::instance().destroy();
}
