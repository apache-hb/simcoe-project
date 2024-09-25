#pragma once

#include "logs/structured/detail.hpp"

namespace sm::logs {
    struct Category {
        const detail::LogCategoryData data;

        Category(detail::LogCategoryData data) noexcept;

        constexpr uint64_t hash() const noexcept { return data.hash; }
    };
}
