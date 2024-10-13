#include "core/macros.h"
#include "stdafx.hpp"

#include "core/string.hpp"

#include "base/panic.h"

#include <filesystem>

using namespace sm;

// NOLINTNEXTLINE
std::string sm::format(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string result = vformat(fmt, args);
    va_end(args);
    return result;
}

std::string sm::vformat(const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);

    int size = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    CTASSERTF(size > 0, "vsnprintf returned %d", size);

    std::string result;
    result.resize(size);

    int res = vsnprintf(result.data(), result.size() + 1, fmt, args);
    CT_UNUSED(res);
    CTASSERTF(res == size, "vsnprintf returned %d, expected %d", res, size);

    return result;
}

template<typename T>
constexpr std::basic_string<T> convertString(const std::filesystem::path& path) {
    if constexpr (std::is_same_v<T, char>) {
        return path.string();
    } else if constexpr (std::is_same_v<T, wchar_t>) {
        return path.wstring();
    }
}

std::string sm::narrow(std::wstring_view wstr) {
    return convertString<char>(wstr);
}

std::wstring sm::widen(std::string_view str) {
    return convertString<wchar_t>(str);
}

std::string sm::trimIndent(StringView str) {
    // trim leading and trailing whitespace
    // also trim leading whitespace before a | character on each line
    size_t start = 0;
    size_t end = str.size();

    while (start < end && isspace(str[start])) {
        ++start;
    }

    while (end > start && isspace(str[end - 1])) {
        --end;
    }

    if (start == end) {
        return "";
    }

    size_t len = end - start;
    std::string result(len, '\0');

    size_t i = 0;
    size_t j = start;
    bool trim = true;

    while (j < end) {
        if (trim && str[j] == '|') {
            ++j;
            trim = false;
        }

        if (j < end) {
            if (!trim && str[j] == '\n') {
                trim = true;
            } else if (trim && isspace(str[j])) {
                ++j;
                continue;
            }

            result[i++] = str[j++];
        }
    }

    result.resize(i);

    return result;
}

StringPair sm::split(std::string_view str, char delim) {
    size_t index = str.find(delim);
    if (index == std::string_view::npos) {
        return {str, {}};
    }

    return {str.substr(0, index), str.substr(index + 1)};
}

std::vector<std::string_view> sm::splitAll(std::string_view str, char delim) {
    if (str.empty())
        return {};

    std::vector<std::string_view> result;
    size_t start = 0;
    size_t index = 0;

    do {
        index = str.find(delim, start);
        result.push_back(str.substr(start, index - start));
        start = index + 1;
    } while (index != std::string_view::npos);

    return result;
}

void sm::trimWhitespace(std::string &str) noexcept {
    while (!str.empty() && isspace(str.front())) {
        str.erase(0, 1);
    }

    while (!str.empty() && isspace(str.back())) {
        str.pop_back();
    }
}

void sm::trimWhitespaceFromView(std::string_view &str) noexcept {
    while (!str.empty() && isspace(str.front())) {
        str.remove_prefix(1);
    }

    while (!str.empty() && isspace(str.back())) {
        str.remove_suffix(1);
    }
}

void sm::replaceAll(std::string &str, std::string_view search, std::string_view replace) {
    size_t pos = 0;
    while ((pos = str.find(search, pos)) != std::string::npos) {
        str.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}
