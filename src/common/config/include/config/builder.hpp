#pragma once

#include "config/init.hpp"

namespace sm::config {
    class OptionBase;
    class Group;

    Group& getCommonGroup() noexcept;

    enum class OptionType {
#define OPTION_TYPE(ID, STR) ID,
#include "config.inc"
    };

    namespace detail {
        template<typename T>
        concept SignedEnum = std::is_enum_v<T> && std::is_signed_v<std::underlying_type_t<T>>;

        template<typename T>
        concept UnsignedEnum = std::is_enum_v<T> && std::is_unsigned_v<std::underlying_type_t<T>>;

        template<typename T>
        constexpr inline OptionType kOptionType = OptionType::eUnknown;

        template<std::signed_integral T>
        constexpr inline OptionType kOptionType<T> = OptionType::eSigned;

        template<std::unsigned_integral T>
        constexpr inline OptionType kOptionType<T> = OptionType::eUnsigned;

        template<std::floating_point T>
        constexpr inline OptionType kOptionType<T> = OptionType::eReal;

        template<>
        constexpr inline OptionType kOptionType<bool> = OptionType::eBoolean;

        template<>
        constexpr inline OptionType kOptionType<std::string> = OptionType::eString;

        template<SignedEnum T>
        constexpr inline OptionType kOptionType<T> = OptionType::eSignedEnum;

        template<UnsignedEnum T>
        constexpr inline OptionType kOptionType<T> = OptionType::eUnsignedEnum;

        template<typename T>
        constexpr inline OptionType kEnumOptionType = OptionType::eUnknown;

        template<std::signed_integral T>
        constexpr inline OptionType kEnumOptionType<T> = OptionType::eSignedEnum;

        template<std::unsigned_integral T>
        constexpr inline OptionType kEnumOptionType<T> = OptionType::eUnsignedEnum;

        template<typename T>
        struct EnumInner { using Type = void; };

        template<SignedEnum T>
        struct EnumInner<T> { using Type = int64_t; };

        template<UnsignedEnum T>
        struct EnumInner<T> { using Type = uint64_t; };

        template<typename T>
        using EnumInnerType = typename EnumInner<T>::Type;

        template<typename T>
        concept IsValidEnum = std::is_enum_v<T> && !std::is_same_v<EnumInnerType<T>, void>;

        struct ConfigBuilder {
            std::string_view name;
            std::string_view description = "no description";

            void init(Name it) noexcept { name = it.value; }
            void init(Description it) noexcept { description = it.value; }
        };

        struct OptionBuilder : ConfigBuilder {
            using ConfigBuilder::init;

            bool readonly = false;
            bool hidden = false;

            OptionType type;
            Group* group = &getCommonGroup();

            void init(ReadOnly it) noexcept { readonly = it.readonly; }
            void init(Hidden it) noexcept { hidden = it.hidden; }
            void init(Category it) noexcept { group = &it.group; }
        };

        template<std::derived_from<ConfigBuilder> T>
        constexpr T buildGroupKwargs(auto&&... args) noexcept {
            T builder{};
            (builder.init(args), ...);
            return builder;
        }

        template<std::derived_from<OptionBuilder> T, typename V> requires (kOptionType<V> != OptionType::eUnknown)
        constexpr T buildOptionKwargs(auto&&... args) noexcept {
            T builder = buildGroupKwargs<T>(args...);
            builder.type = kOptionType<V>;
            return builder;
        }
    }
}
