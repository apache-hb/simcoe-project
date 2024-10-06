#pragma once

#include "logs/structured/channel.hpp"
#include "db/connection.hpp"

#include "core/fs.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    void setup(db::Connection connection);
    void shutdown();

    bool isConsoleAvailable() noexcept;
    ILogChannel *console();

    bool isDebugConsoleAvailable() noexcept;
    ILogChannel *debugConsole();

    ILogChannel *file(const fs::path& path);
    ILogChannel *database(db::Connection connection);
}
