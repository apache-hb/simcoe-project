#pragma once

#include "logs/structured/logging.hpp"

#include "core/fs.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    void setup(db::Connection& connection);
    void shutdown();

    ILogChannel *console();
    ILogChannel *file(const fs::path& path);
    ILogChannel *database(db::Connection& connection);
}
