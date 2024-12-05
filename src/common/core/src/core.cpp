#include "stdafx.hpp"

#include "base/panic.h"

#include "core/macros.hpp"

using namespace std::string_view_literals;

#ifdef _WIN32
#   define SM_ALIGNED_MALLOC _aligned_malloc
#   define SM_ALIGNED_FREE _aligned_free
#else
#   define SM_ALIGNED_MALLOC std::aligned_alloc
#   define SM_ALIGNED_FREE free
#endif

void *operator new(size_t size) throw() {
    void *ptr = std::malloc(size);
    CTASSERTF(ptr != nullptr, "Failed to allocate %zu bytes", size);
    return ptr;
}

void operator delete(void *ptr) noexcept {
    if (ptr == nullptr)
        return;

    std::free(ptr);
}

void operator delete(void *ptr, size_t size) noexcept {
    if (ptr == nullptr)
        return;

    std::free(ptr);
}

void *operator new(size_t size, std::align_val_t align) throw() {
    void *ptr = SM_ALIGNED_MALLOC((size_t)align, size);
    CTASSERTF(ptr != nullptr, "Failed to allocate %zu bytes with alignment %zu", size, (size_t)align);
    return ptr;
}

void operator delete(void *ptr, std::align_val_t align) noexcept {
    if (ptr == nullptr)
        return;

    SM_ALIGNED_FREE(ptr);
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
    for (size_t j = 0; j < name.length(); ++j) {
        if (name[j] == ':') {
            dst[i++] = '.';
            ++j;
        } else {
            dst[i++] = name[j];
        }
    }

    return i - 1;
}
