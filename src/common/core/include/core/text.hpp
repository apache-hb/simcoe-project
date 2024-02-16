#pragma once

#include "core/arena.hpp"

#include <cstdarg>

#include <string>

namespace sm {
    using StringArena = StandardArena<char>;
    using WideStringArena = StandardArena<wchar_t>;
    using String = std::basic_string<char, std::char_traits<char>, StringArena>;
    using WideString = std::basic_string<wchar_t, std::char_traits<wchar_t>, WideStringArena>;

    using StringView = std::string_view;
    using WideStringView = std::wstring_view;

    String format(const char *fmt, ...);
    String vformat(const char *fmt, va_list args);

    String narrow(std::wstring_view str);
    WideString widen(std::string_view str);
}
