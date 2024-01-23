#pragma once

#include <cstdarg>

#include <string>

namespace sm {
    std::string format(const char *fmt, ...);
    std::string vformat(const char *fmt, va_list args);

    std::string narrow(std::wstring_view str);
    std::wstring widen(std::string_view str);
}
