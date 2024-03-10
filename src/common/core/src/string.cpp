#include "core/string.hpp"

#include "base/panic.h"

using namespace sm;

// NOLINTNEXTLINE
sm::String sm::format(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sm::String result = vformat(fmt, args);
    va_end(args);
    return result;
}

sm::String sm::vformat(const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);

    int size = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    CTASSERTF(size > 0, "vsnprintf returned %d", size);

    sm::String result;
    result.resize(size);

    int res = vsnprintf(result.data(), result.size() + 1, fmt, args);
    CTASSERTF(res == size, "vsnprintf returned %d, expected %d", res, size);

    return result;
}

sm::String sm::narrow(std::wstring_view wstr) {
    sm::String result(wstr.size() + 1, '\0');
    size_t size = result.size();

    errno_t err = wcstombs_s(&size, result.data(), result.size(), wstr.data(), wstr.size());
    if (err != 0) {
        return "";
    }

    result.resize(size - 1);
    return result;
}

sm::WideString sm::widen(std::string_view str) {
    // use mbstowcs_s to get the size of the buffer
    size_t size = 0;
    errno_t err = mbstowcs_s(&size, nullptr, 0, str.data(), str.size());

    if (err != 0) {
        return L"";
    }

    sm::WideString result(size, '\0');
    err = mbstowcs_s(&size, result.data(), result.size(), str.data(), str.size());

    if (err != 0) {
        return L"";
    }

    result.resize(size - 1);
    return result;
}
