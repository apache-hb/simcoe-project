#include "threads/threads.hpp"

#include "backends/common.hpp"
#include <unordered_map>

using namespace sm::threads;

ThreadHandle::ThreadHandle(HANDLE handle, DWORD threadId) noexcept
    : mHandle(handle)
    , mThreadId(threadId)
{ }

ThreadHandle::~ThreadHandle() noexcept {
    if (auto result = join()) {
        LOG_ERROR(ThreadLog, "Failed to join thread: {}", result);
    }

    CloseHandle(mHandle);
}

sm::OsError ThreadHandle::join() noexcept {
    DWORD result = WaitForSingleObject(mHandle, INFINITE);
    switch (result) {
    case WAIT_OBJECT_0: return OsError(ERROR_SUCCESS);
    case WAIT_TIMEOUT: return OsError(ERROR_TIMEOUT);
    default: return OsError(GetLastError());
    }
}

static std::vector<Cache> getCachesOfLevel(const std::vector<Cache>& caches, uint32_t level) {
    std::vector<Cache> result;
    for (const auto& cache : caches) {
        if (cache.level != level)
            continue;

        result.push_back(cache);
    }

    return result;
}

std::vector<Cache> ICpuGeometry::l3Caches() const {
    return getCachesOfLevel(mCaches, 3);
}

std::vector<Cache> ICpuGeometry::l2Caches() const {
    return getCachesOfLevel(mCaches, 2);
}

std::vector<Cache> ICpuGeometry::l1Caches() const {
    return getCachesOfLevel(mCaches, 1);
}

void ICpuGeometry::createCoreGroups() {
    std::unordered_map<WORD, Group> groups;

    for (const auto& core : mLogicalCores) {
        Group& group = groups[core.group];
        group.logicalCores.push_back(core);
    }

    for (auto& [id, group] : groups) {
        mCoreGroups.push_back(group);
    }
}
