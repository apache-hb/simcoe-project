#pragma once

#include <string>

#include <stdarg.h>

namespace sm {
    using String = std::string;
    using WideString = std::wstring;

    using StringView = std::string_view;
    using WideStringView = std::wstring_view;

    String format(const char *fmt, ...);
    String vformat(const char *fmt, va_list args);

    String narrow(std::wstring_view str);
    WideString widen(std::string_view str);
}
