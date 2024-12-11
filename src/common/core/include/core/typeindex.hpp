#pragma once

#include "core/core.hpp"

#include <type_traits>

namespace sm {
    uint32_t next_index() noexcept;

    template <typename T>
    class TypeIndex {
    public:
        static uint32_t index() noexcept {
            static uint32_t index = next_index();
            return index;
        }
    };
}

template<typename T>
struct std::hash<sm::TypeIndex<T>> {
    size_t operator()(const sm::TypeIndex<T>& index) const {
        return index.index();
    }
};
