#pragma once

#include <string>

#include <stdarg.h>

namespace sm {
    template<typename T>
    using CoreString = std::basic_string<T>;

    template<typename T>
    using CoreStringView = std::basic_string_view<T>;

    using String = CoreString<char>;
    using WideString = CoreString<wchar_t>;

    using StringView = CoreStringView<char>;
    using WideStringView = CoreStringView<wchar_t>;

    String format(const char *fmt, ...);
    String vformat(const char *fmt, va_list args);

    String narrow(std::wstring_view str);
    WideString widen(std::string_view str);

    String trimIndent(StringView str);

    struct StringPair {
        std::string_view left;
        std::string_view right;
    };

    StringPair split(StringView str, char delim);
}
