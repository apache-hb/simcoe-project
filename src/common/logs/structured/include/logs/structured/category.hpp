#pragma once

#include "logs/structured/detail.hpp"

namespace sm::logs::structured {
    struct CategoryInfo {
        std::string_view name;
        uint64_t hash;

        consteval CategoryInfo(std::string_view name, uint64_t line) noexcept
            : name(name)
            , hash(detail::hashMessage(name, line))
        { }
    };

    namespace detail {
        struct CategoryId {
            const CategoryInfo data;

            CategoryId(CategoryInfo data) noexcept;

            constexpr uint64_t hash() const noexcept { return data.hash; }
        };

        template<typename T>
        inline CategoryId gLogCategory = T{};
    }
}
