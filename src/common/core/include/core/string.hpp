#pragma once

#include <string>

#include <stdarg.h>
#include <vector>

namespace sm {
    template<typename T>
    using CoreString = std::basic_string<T>;

    template<typename T>
    using CoreStringView = std::basic_string_view<T>;

    using String = CoreString<char>;
    using WideString = CoreString<wchar_t>;

    using StringView = CoreStringView<char>;
    using WideStringView = CoreStringView<wchar_t>;

    std::string format(const char *fmt, ...);
    std::string vformat(const char *fmt, va_list args);

    std::string narrow(std::wstring_view str);
    std::wstring widen(std::string_view str);

    std::string trimIndent(std::string_view str);

    struct StringPair {
        std::string_view left;
        std::string_view right;
    };

    StringPair split(std::string_view str, char delim);

    std::vector<std::string_view> splitAll(std::string_view str, char delim);

    void trimWhitespace(std::string &str) noexcept;

    void replaceAll(std::string &str, std::string_view search, std::string_view replace);
}
