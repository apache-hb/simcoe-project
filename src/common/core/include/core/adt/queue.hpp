#pragma once

#include "core/adt/vector.hpp"

#include <algorithm>

namespace sm {
    template<typename T, typename C = std::less<T>>
    class PriorityQueue {
        using Storage = sm::Vector<T>;
        Storage mStorage;

    public:
        using ConstIterator = typename Storage::const_iterator;

        constexpr void emplace(auto&&... args) noexcept {
            mStorage.emplace_back(std::forward<decltype(args)>(args)...);
            std::push_heap(mStorage.begin(), mStorage.end(), C{});
        }

        constexpr T pop() noexcept {
            std::pop_heap(mStorage.begin(), mStorage.end(), C{});
            T result = std::move(mStorage.back());
            mStorage.pop_back();
            return result;
        }

        constexpr const T& top() const noexcept {
            return mStorage.front();
        }

        constexpr bool is_empty() const noexcept {
            return mStorage.empty();
        }

        constexpr size_t length() const noexcept {
            return mStorage.size();
        }

        constexpr ConstIterator begin() const noexcept {
            return mStorage.begin();
        }

        constexpr ConstIterator end() const noexcept {
            return mStorage.end();
        }

        constexpr T& operator[](size_t index) noexcept {
            return mStorage[index];
        }

        constexpr const T& operator[](size_t index) const noexcept {
            return mStorage[index];
        }
    };
}
