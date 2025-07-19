#pragma once

#include "base/traits.hpp"

#include <chrono>
#include <vector>
#include <string>
#include <string_view>

namespace sm::db {
    using Blob = std::vector<std::uint8_t>;
    using DateTime = std::chrono::time_point<std::chrono::system_clock>;
    using DateTimeTZ = std::chrono::zoned_time<std::chrono::system_clock>;
    using Duration = std::chrono::milliseconds;
}

namespace sm::dao {
    enum class ColumnType : uint_least8_t {
        eUnknown,  ///< unknown type

        eInt,      ///< int32
        eUint,     ///< uint32,
        eLong,     ///< int64
        eUlong,    ///< uint64
        eBool,     ///< bool

        /// @note variable length string
        eVarChar,  ///< std::string

        /// @note fixed length string
        eChar,     ///< std::string

        eFloat,    ///< float
        eDouble,   ///< double
        eBlob,     ///< Blob
        eDateTime, ///< DateTime
        eDateTimeWithTZ, ///< DateTime with TZ info
        eDuration, ///< Duration

        eCount
    };

    std::string_view toString(ColumnType type) noexcept;

    template<typename T>
    struct ColumnTypeOf;

    template<>
    struct ColumnTypeOf<int32_t> {
        static constexpr ColumnType value = ColumnType::eInt;
    };

    template<>
    struct ColumnTypeOf<uint32_t> {
        static constexpr ColumnType value = ColumnType::eUint;
    };

    template<>
    struct ColumnTypeOf<int64_t> {
        static constexpr ColumnType value = ColumnType::eLong;
    };

    template<>
    struct ColumnTypeOf<uint64_t> {
        static constexpr ColumnType value = ColumnType::eUlong;
    };

    template<>
    struct ColumnTypeOf<bool> {
        static constexpr ColumnType value = ColumnType::eBool;
    };

    template<>
    struct ColumnTypeOf<std::string> {
        static constexpr ColumnType value = ColumnType::eVarChar;
    };

    template<>
    struct ColumnTypeOf<std::string_view> {
        static constexpr ColumnType value = ColumnType::eFloat;
    };

    template<>
    struct ColumnTypeOf<float> {
        static constexpr ColumnType value = ColumnType::eFloat;
    };

    template<>
    struct ColumnTypeOf<double> {
        static constexpr ColumnType value = ColumnType::eDouble;
    };

    template<>
    struct ColumnTypeOf<db::Blob> {
        static constexpr ColumnType value = ColumnType::eBlob;
    };

    template<>
    struct ColumnTypeOf<db::DateTime> {
        static constexpr ColumnType value = ColumnType::eDateTime;
    };

    template<>
    struct ColumnTypeOf<db::DateTimeTZ> {
        static constexpr ColumnType value = ColumnType::eDateTimeWithTZ;
    };

    template<typename T> requires IsSpecialization<std::chrono::duration, T>
    struct ColumnTypeOf<T> {
        static constexpr ColumnType value = ColumnType::eDuration;
    };

    template<typename T>
    constexpr ColumnType kColumnType = ColumnTypeOf<T>::value;

    constexpr bool isStringType(ColumnType type) noexcept {
        return type == ColumnType::eVarChar || type == ColumnType::eChar;
    }

    enum class AutoIncrement : uint_least8_t {
        eNever, // never auto-increment
        eAlways, // always auto-increment, prevents manual setting
        eDefault, // auto-increment if not set

        eCount
    };

    enum class OnDelete : uint_least8_t {
        eRestrict,
        eSetNull,
        eCascade,

        eCount
    };

    enum class OnUpdate : uint_least8_t {
        eRestrict,
        eCascade,

        eCount
    };
}
