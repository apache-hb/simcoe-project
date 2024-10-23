#pragma once

#include "fmtlib/format.h"
#include "core/win32.hpp"
#include "core/fs.hpp"

#include "db/db.hpp"

namespace sm::launch {
    struct LaunchCleanup {
        ~LaunchCleanup() noexcept;
    };

    struct LaunchInfo {
        db::DbType logDbType;
        db::ConnectionConfig logDbConfig;
        fs::path logPath;

        bool threads = true;
        bool network = false;
    };

    LaunchCleanup commonInit(HINSTANCE hInstance, const LaunchInfo& info);
}
