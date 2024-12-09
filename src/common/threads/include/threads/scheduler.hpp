#pragma once

#include "core/error.hpp"
#include "core/macros.hpp"
#include "core/defer.hpp"

#include "threads/threads.hpp"

namespace sm::threads {
    class ThreadHandle {
        std::stop_source mStop;
        system::os::Thread mHandle = system::os::kInvalidThread;
        system::os::ThreadId mThreadId = 0;

    public:
        ThreadHandle(system::os::Thread handle, system::os::ThreadId threadId);
        ~ThreadHandle() noexcept;

        ThreadHandle(ThreadHandle &&other) noexcept {
            swap(*this, other);
        }

        ThreadHandle &operator=(ThreadHandle &&other) noexcept {
            swap(*this, other);
            return *this;
        }

        SM_NOCOPY(ThreadHandle);

        bool isStopping() const noexcept { return mStop.stop_requested(); }
        bool stop() noexcept { return mStop.request_stop(); }

        OsError join() noexcept;

        system::os::Thread getHandle() const noexcept { return mHandle; }
        system::os::ThreadId getThreadId() const noexcept { return mThreadId; }

        friend void swap(ThreadHandle &lhs, ThreadHandle &rhs) noexcept {
            std::swap(lhs.mStop, rhs.mStop);
            std::swap(lhs.mHandle, rhs.mHandle);
            std::swap(lhs.mThreadId, rhs.mThreadId);
        }
    };

    class IScheduler {
        virtual ThreadHandle launchThread(void *param, system::os::StartRoutine start) = 0;

    public:
        virtual ~IScheduler() = default;

        template<typename F>
        ThreadHandle launch(F &&fn) {
            auto thunk = [](void *param) noexcept -> unsigned long {
                auto fn = std::make_unique<F>(reinterpret_cast<F*>(param));

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

            // launchThread frees this when the thread exits, unless creation fails
            // in which case we need to cleanup
            F *param = new F(std::forward<F>(fn));
            errdefer { delete param; };

            return launchThread(param, thunk);
        }
    };
}
