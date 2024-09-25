#pragma once

#include <string_view>

namespace sm::logs::detail {
    consteval uint64_t hashMessage(std::string_view message, uint64_t seed) noexcept {
        uint64_t hash = seed;
        for (char c : message) {
            hash = (hash * 31) + c;
        }
        return hash;
    }

    struct LogCategoryData {
        std::string_view name;
        uint64_t hash;

        consteval LogCategoryData(std::string_view name, uint64_t line) noexcept
            : name(name)
            , hash(hashMessage(name, line))
        { }
    };
}
