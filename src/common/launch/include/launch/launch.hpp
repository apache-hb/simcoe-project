#pragma once

#include "fmtlib/format.h"
#include "core/win32.hpp"
#include "core/fs.hpp"

#include "db/db.hpp"

namespace sm::launch {
    struct LaunchResult {
        ~LaunchResult() noexcept;

        bool shouldExit() const noexcept;
        int exitCode() const noexcept;
    };

    struct LaunchInfo {
        db::DbType logDbType;
        db::ConnectionConfig logDbConfig;

        fs::path logPath;

        db::DbType infoDbType;
        db::ConnectionConfig infoDbConfig;

        bool threads = true;
        bool network = false;
    };

    LaunchResult commonInit(HINSTANCE hInstance, const LaunchInfo& info);
    LaunchResult commonInitMain(int argc, const char **argv, const LaunchInfo& info);
    LaunchResult commonInitWinMain(HINSTANCE hInstance, int nShowCmd, const LaunchInfo& info);
}
