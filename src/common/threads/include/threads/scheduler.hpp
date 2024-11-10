#pragma once

#include "core/error.hpp"

#include "core/memory/unique.hpp"

#include "core/win32.hpp" // IWYU pragma: keep

#include "threads/threads.hpp"

namespace sm::threads {
    class ThreadHandle {
        HANDLE mHandle;
        DWORD mThreadId;

    public:
        ThreadHandle(HANDLE handle, DWORD threadId) noexcept;

        ~ThreadHandle() noexcept;

        OsError join() noexcept;

        HANDLE getHandle() const noexcept { return mHandle; }
        DWORD getThreadId() const noexcept { return mThreadId; }
    };

    class IScheduler {
        virtual ThreadHandle launchThread(void *param, LPTHREAD_START_ROUTINE start) = 0;

    public:
        virtual ~IScheduler() = default;

        template<typename F>
        ThreadHandle launch(F &&fn) {
            auto thunk = [](void *param) noexcept -> unsigned long {
                auto fn = sm::makeUnique<F>(reinterpret_cast<F*>(param));

                try {
                    (*fn)();
                    return 0;
                } catch (std::exception &err) {
                    LOG_ERROR(ThreadLog, "Thread exception: {}", err.what());
                } catch (...) {
                    LOG_ERROR(ThreadLog, "Unknown thread exception");
                }

                return -1;
            };

            // launchThread frees this when the thread exits
            F *param = new F(std::forward<F>(fn));
            return launchThread(param, thunk);
        }
    };
}
