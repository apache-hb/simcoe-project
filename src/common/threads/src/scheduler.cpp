#include "threads/scheduler.hpp"

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
