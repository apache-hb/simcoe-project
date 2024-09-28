#include "common.hpp"

namespace threads = sm::threads;
namespace detail = sm::threads::detail;

class DefaultScheduler final : public threads::IScheduler {
    threads::ThreadHandle launchThread(void *param, LPTHREAD_START_ROUTINE start) override {
        DWORD id = 0;
        HANDLE thread = CreateThread(nullptr, 0, start, param, 0, &id);
        if (thread == nullptr) {
            auto error = sm::OsError(GetLastError());
            LOG_ERROR(ThreadLog, "Failed to create thread: {}", error);
            error.raise();
        }


        return threads::ThreadHandle(thread, id);
    }
public:
    DefaultScheduler() noexcept = default;
};

class DefaultCpuGeometry final : public threads::ICpuGeometry {
    threads::IScheduler *newScheduler(std::vector<threads::LogicalCore> threads) override {
        return new DefaultScheduler();
    }

public:
    DefaultCpuGeometry() noexcept = default;
};

threads::ICpuGeometry *detail::newDefaultGeometry() {
    return new DefaultCpuGeometry();
}
