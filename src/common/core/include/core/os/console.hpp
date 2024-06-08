#pragma once

#include <string_view>

#include "core/adt/zstring_view.hpp"

namespace sm::os {
    struct ConsoleHandle;

    class Console final {
        ConsoleHandle *mHandle;

        Console(ConsoleHandle *handle) noexcept
            : mHandle(handle)
        { }

    public:
        void print(std::string_view message) noexcept;
        bool valid() const noexcept;

        static Console get() noexcept;
    };

    bool isDebuggerAttached() noexcept;
    void printDebugMessage(ZStringView message) noexcept;
}
