#include "stdafx.hpp"

#include "core/os/console.hpp"

using namespace sm;

bool os::isDebuggerAttached() noexcept {
    return false;
}

void os::printDebugMessage(ZStringView message) noexcept {
    (void)fprintf(stderr, "%.*s\n", static_cast<int>(message.size()), message.data());
}

os::Console os::Console::get() noexcept {
    return os::Console { nullptr };
}

void os::Console::print(std::string_view message) noexcept {
    (void)fprintf(stdout, "%.*s\n", static_cast<int>(message.size()), message.data());
}

bool os::Console::valid() const noexcept {
    return true;
}
