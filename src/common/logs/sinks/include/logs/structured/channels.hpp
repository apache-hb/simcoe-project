#pragma once

#include "logs/structured/channel.hpp"
#include "db/connection.hpp"

#include "core/fs.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    void create(db::Connection connection);
    void destroy(void) noexcept;

    bool addConsoleChannel();
    bool addDebugChannel();
    bool addFileChannel(const fs::path& path);
    bool addDatabaseChannel(db::Connection connection);

    bool isConsoleAvailable() noexcept;
    ILogChannel *console();

    bool isDebugConsoleAvailable() noexcept;
    ILogChannel *debugConsole();

    ILogChannel *file(const fs::path& path);
    IAsyncLogChannel *database(db::Connection connection);
}
