#pragma once

#include <bit>

namespace sm {
    enum class ByteOrder {
        /// big endian byte order (ppc64, sparcv9)
        eBig = __ORDER_BIG_ENDIAN__,

        /// little endian byte order (x86_64)
        eLittle = __ORDER_LITTLE_ENDIAN__,

        /// byte order of the current platform
        eNative = __BYTE_ORDER__,
    };

    /**
     * @brief an endian type
     *
     * Endian aware value for storing types with a consistent byte order.
     *
     * @tparam T the type of the value
     * @tparam order the desired byte order of the value
     */
    template<typename T, ByteOrder Order>
    struct EndianValue {
        constexpr EndianValue() noexcept = default;
        constexpr EndianValue(T v) noexcept
            : underlying((Order == ByteOrder::eNative) ? v : std::byteswap(v))
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
            return (Order == ByteOrder::eNative) ? underlying : std::byteswap(underlying);
        }

        constexpr EndianValue& operator=(T v) noexcept {
            underlying = (Order == ByteOrder::eNative) ? v : std::byteswap(v);
            return *this;
        }

        constexpr T little() const noexcept {
            return (Order == ByteOrder::eLittle) ? underlying : std::byteswap(underlying);
        }

        constexpr T big() const noexcept {
            return (Order == ByteOrder::eBig) ? underlying : std::byteswap(underlying);
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
    using be = EndianValue<T, ByteOrder::eBig>;

    /**
     * @brief a little endian value
     *
     * @tparam T the type of the value
     */
    template<typename T>
    using le = EndianValue<T, ByteOrder::eLittle>;
}
