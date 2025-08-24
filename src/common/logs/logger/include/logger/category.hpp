#pragma once

#include "logger/detail.hpp"

namespace sm::logs {
    struct CategoryInfo {
        std::string_view name;
        uint64_t hash;

        consteval CategoryInfo(std::string_view id) noexcept
            : name(id)
            , hash(detail::hashMessage(id))
        { }
    };

    namespace detail {
        struct CategoryId {
            const CategoryInfo data;

            CategoryId(CategoryInfo info) noexcept;

            constexpr uint64_t hash() const noexcept { return data.hash; }
            constexpr bool operator==(const CategoryInfo& info) const noexcept { return data.hash == info.hash; }
        };

        template<typename T>
        inline CategoryId gLogCategory = T{};
    }
}
