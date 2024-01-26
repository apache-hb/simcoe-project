#pragma once

#include <cstdarg>

#include <string>

namespace sm {
    std::string format(const char *fmt, ...);
    std::string vformat(const char *fmt, va_list args);

    std::string narrow(std::wstring_view str);
    std::wstring widen(std::string_view str);

    struct SplitView {
        std::string_view left;
        std::string_view right;

        constexpr SplitView(std::string_view str, char delim) {
            size_t pos = str.find(delim);
            if (pos == std::string_view::npos) {
                left = str;
                right = std::string_view();
            } else {
                left = str.substr(0, pos);
                right = str.substr(pos + 1);
            }
        }

        constexpr operator bool() const { return !left.empty(); }
    };
}
