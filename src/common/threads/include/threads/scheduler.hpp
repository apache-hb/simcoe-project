#pragma once

#include "core/error.hpp"

#include "core/memory/unique.hpp"

#include "system/threads.hpp"

#include "threads/threads.hpp"

namespace sm::threads {
    class ThreadHandle {
        system::os::Thread mHandle;
        system::os::ThreadId mThreadId;

    public:
        ThreadHandle(system::os::Thread handle, system::os::ThreadId threadId) noexcept;

        ~ThreadHandle() noexcept;

        OsError join() noexcept;

        system::os::Thread getHandle() const noexcept { return mHandle; }
        system::os::ThreadId getThreadId() const noexcept { return mThreadId; }
    };

    class IScheduler {
        virtual ThreadHandle launchThread(void *param, system::os::StartRoutine start) = 0;

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
