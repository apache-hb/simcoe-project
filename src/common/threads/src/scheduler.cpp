#include "threads/scheduler.hpp"

using namespace sm::threads;

ThreadHandle::ThreadHandle(system::os::Thread handle, system::os::ThreadId threadId)
    : mHandle(handle)
    , mThreadId(threadId)
{ }

ThreadHandle::~ThreadHandle() noexcept {
    if (mHandle != system::os::kInvalidThread) {
        if (auto result = join()) {
            LOG_ERROR(ThreadLog, "Failed to join thread: {}", result);
        }

        system::os::destroyThread(mHandle);
    }
}

sm::OsError ThreadHandle::join() noexcept {
    if (!isStopping()) {
        stop();
    }

    return system::os::joinThread(mHandle);
}
