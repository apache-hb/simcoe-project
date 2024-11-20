#pragma once

#include "core/core.hpp"

#include "core/win32.hpp" // IWYU pragma: keep

#include "logs/logging.hpp"

LOG_MESSAGE_CATEGORY(ThreadLog, "Threads");

namespace sm::db {
    class Connection;
}

namespace sm::threads {
    static constexpr size_t kMaxThreads = 512;

    enum class ThreadClass : uint8 {
        eRealtime,
        eNormal,
        eWorker,
        eIdle
    };

    enum class PriorityClass : int16 {
        eHigh = HIGH_PRIORITY_CLASS,
        eNormal = NORMAL_PRIORITY_CLASS,
        eIdle = THREAD_PRIORITY_IDLE,
    };

    enum class CacheType : uint8 {
        eUnified,
        eInstruction,
        eData,
        eTrace,
        eUnknown
    };

    void create(void);
    void destroy(void) noexcept;
}
