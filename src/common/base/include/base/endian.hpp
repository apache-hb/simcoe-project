#pragma once

#include <bit>
#include <concepts>

namespace sm {
    /**
     * @brief an endian type
     *
     * Endian aware value for storing types with a consistent byte order.
     *
     * @tparam T the type of the value
     * @tparam order the desired byte order of the value
     */
    template<std::integral T, std::endian Order>
    struct EndianValue {
        constexpr EndianValue() noexcept = default;
        constexpr EndianValue(T v) noexcept
            : underlying((Order == std::endian::native) ? v : std::byteswap(v))
        { }

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
            return (Order == std::endian::native) ? underlying : std::byteswap(underlying);
        }

        constexpr EndianValue& operator=(T v) noexcept {
            underlying = (Order == std::endian::native) ? v : std::byteswap(v);
            return *this;
        }

        constexpr T little() const noexcept {
            return (Order == std::endian::little) ? underlying : std::byteswap(underlying);
        }

        constexpr T big() const noexcept {
            return (Order == std::endian::big) ? underlying : std::byteswap(underlying);
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
