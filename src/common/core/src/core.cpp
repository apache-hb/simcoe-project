#include "stdafx.hpp"

#include "core/core.hpp"
#include "core/macros.hpp"

#include "core/win32.hpp"
#include <Psapi.h>

using namespace std::string_view_literals;

static MODULEINFO getModuleInfo() {
    MODULEINFO result = {0};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &result, sizeof(result));
    return result;
}

bool sm::isInStaticStorage(const void *ptr) noexcept {
    static MODULEINFO info = getModuleInfo();
    return ptr >= info.lpBaseOfDll && ptr < (char*)info.lpBaseOfDll + info.SizeOfImage;
}

static void trimPrefix(std::string_view &name, std::string_view prefix) {
    if (name.starts_with(prefix)) {
        name.remove_prefix(prefix.size());
    }
}

static void trimSuffix(std::string_view &name, std::string_view suffix) {
    if (name.ends_with(suffix)) {
        name.remove_suffix(suffix.size());
    }
}

std::string_view sm::trimTypeName(std::string_view name) {
    trimPrefix(name, "const "sv);
    trimPrefix(name, "struct "sv);
    trimPrefix(name, "class "sv);
    trimPrefix(name, "enum "sv);

    while (name.ends_with(" ") || name.ends_with("&") || name.ends_with("*")) {
        name.remove_suffix(1);
    }

    trimSuffix(name, " const"sv);

    return name;
}

size_t sm::cleanTypeName(char *dst, std::string_view name) {
    size_t i = 0;
    for (size_t j = 0; j < name.length() - 1; ++j) {
        if (name[j] == ':') {
            dst[i++] = '.';
            ++j;
        } else {
            dst[i++] = name[j];
        }
    }

    return i;
}
