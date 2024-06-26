#include "stdafx.hpp"

#include "core/os/console.hpp"

using namespace sm;

bool os::isDebuggerAttached() noexcept {
    return IsDebuggerPresent();
}

void os::printDebugMessage(ZStringView message) noexcept {
    OutputDebugStringA(message.c_str());
}

os::Console os::Console::get() noexcept {
    return os::Console { (ConsoleHandle*)GetStdHandle(STD_OUTPUT_HANDLE) };
}

void os::Console::print(std::string_view message) noexcept {
    DWORD written;
    WriteConsoleA(mHandle, message.data(), static_cast<DWORD>(message.size()), &written, nullptr);

    CTASSERTF(written == message.size(), "Failed to write all bytes to console (%lu/%zu)", written, message.size());
}

bool os::Console::valid() const noexcept {
    return mHandle != INVALID_HANDLE_VALUE;
}
