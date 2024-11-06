#pragma once

#include <coroutine>
#include <exception>

namespace sm::threads {
    class Executor {

    };

    template<typename T>
    class Task {
        class Handle : public std::coroutine_handle<class Promise> {
        public:
            bool await_ready() const;
            void await_suspend(std::coroutine_handle<> handle);
            void await_resume() const;
        };

        class Promise {
            T mValue;
            std::exception_ptr mException;

        public:
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void unhandled_exception() noexcept {
                mException = std::current_exception();
            }

            Handle get_return_object() { return Handle::from_promise(*this); }
            void return_void() noexcept { }
        };

        using handle_type = Handle;
        using promise_type = Promise;
    };
}
