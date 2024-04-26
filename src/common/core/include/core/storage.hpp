#pragma once

#include <utility>

namespace sm {
    template<typename T, size_t TSize = sizeof(T), size_t TAlign = alignof(T)>
    class Storage {
        alignas(TAlign) char mStorage[TSize];

    public:
        constexpr Storage() noexcept = default;
        constexpr ~Storage() noexcept {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                data()->~T();
            }
        }

        constexpr Storage(const Storage& other) noexcept {
            memcpy(mStorage, other.mStorage, sizeof(mStorage));
        }

        constexpr Storage(Storage&& other) noexcept {
            memcpy(mStorage, other.mStorage, sizeof(mStorage));
        }

        constexpr Storage& operator=(const Storage& other) noexcept {
            if (this == &other) return *this;

            memcpy(mStorage, other.mStorage, sizeof(mStorage));
            return *this;
        }

        constexpr Storage& operator=(Storage&& other) noexcept {
            memcpy(mStorage, other.mStorage, sizeof(mStorage));
            return *this;
        }

        constexpr void *ptr() noexcept { return mStorage; }
        constexpr const void *ptr() const noexcept { return mStorage; }

        constexpr T *data() noexcept { return reinterpret_cast<T*>(mStorage); }
        constexpr const T *data() const noexcept { return reinterpret_cast<const T*>(mStorage); }

        constexpr T& ref() noexcept { return *data(); }
        constexpr const T& ref() const noexcept { return *data(); }

        constexpr T& operator=(T&& other) noexcept {
            return *data() = std::move(other);
        }

        friend constexpr void swap(Storage& lhs, Storage& rhs) noexcept {
            std::swap(lhs.mStorage, rhs.mStorage);
        }
    };
}
