#pragma once

#include <bit>
#include <concepts>

#include "core/digit.hpp"

namespace sm {
    template<size_t N, typename T>
    constexpr SizedInteger<N, T> byteswap(SizedInteger<N, T> value) noexcept {
        return value.byteswap();
    }

    template<std::integral T>
    constexpr T byteswap(T value) noexcept {
        return std::byteswap(value);
    }

    /**
     * @brief an endian type
     *
     * Endian aware value for storing types with a consistent byte order.
     *
     * @tparam T the type of the value
     * @tparam order the desired byte order of the value
     */
    template<typename T, std::endian Order>
    struct EndianValue {
        constexpr EndianValue() noexcept = default;
        constexpr EndianValue(T v) noexcept {
            store(v);
        }

        /**
         * @brief get the value converted to the platforms native byte ordering
         * @see EndianValue::get
         * @return T the value
         */
        constexpr operator T() const noexcept {
            return load();
        }

        /**
         * @brief get the value converted to the platforms byte ordering
         *
         * @return T the converted value
         */
        constexpr T load() const noexcept {
            if constexpr (Order == std::endian::native) {
                return underlying;
            } else {
                return byteswap(underlying);
            }
        }

        constexpr void store(T v) noexcept {
            if constexpr (Order == std::endian::native) {
                underlying = v;
            } else {
                underlying = byteswap(v);
            }
        }

        constexpr EndianValue& operator=(T v) noexcept {
            store(v);
            return *this;
        }

        constexpr T little() const noexcept {
            if constexpr (Order == std::endian::little) {
                return underlying;
            } else {
                return byteswap(underlying);
            }
        }

        constexpr T big() const noexcept {
            if constexpr (Order == std::endian::big) {
                return underlying;
            } else {
                return byteswap(underlying);
            }
        }

        template<typename V> requires (sizeof(V) == sizeof(T))
        constexpr V as() const noexcept {
            return std::bit_cast<V>(load());
        }

        /// the native value
        T underlying;
    };

    /**
     * @brief a big endian value
     *
     * @tparam T the type of the value
     */
    template<typename T>
    using be = EndianValue<T, std::endian::big>;

    /**
     * @brief a little endian value
     *
     * @tparam T the type of the value
     */
    template<typename T>
    using le = EndianValue<T, std::endian::little>;
}
