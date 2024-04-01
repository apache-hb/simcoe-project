#pragma once

#include "core/core.hpp"

namespace sm {
    uint32 next_index();

    template <typename T>
    class TypeIndex {
    public:
        static uint32 index() {
            static uint32 index = next_index();
            return index;
        }
    };
}

template<typename T>
struct std::hash<sm::TypeIndex<T>> {
    std::size_t operator()(const sm::TypeIndex<T>& index) const {
        return index.index();
    }
};
