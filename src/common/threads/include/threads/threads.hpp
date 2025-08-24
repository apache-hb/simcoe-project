#pragma once

#include "core/core.hpp"

#include "system/threads.hpp"

#include "logger/logging.hpp"

LOG_MESSAGE_CATEGORY(ThreadLog, "Threads");

namespace sm::db {
    class Connection;
}

namespace sm::threads {
    static constexpr size_t kMaxThreads = 512;

    enum class ThreadClass : uint8_t {
        eRealtime,
        eNormal,
        eWorker,
        eIdle
    };

    enum class PriorityClass : int64_t {
        eHigh = system::os::kHighPriorityClass,
        eNormal = system::os::kNormalPriorityClass,
        eIdle = system::os::kIdlePriorityClass,
    };

    enum class CacheType : uint8_t {
        eUnified,
        eInstruction,
        eData,
        eTrace,
        eUnknown
    };

    void create(void);
    void destroy(void) noexcept;
}
