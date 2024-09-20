#pragma once

#include <simcoe_config.h>

#include <atomic>
#include <mutex>
#include <span>
#include <string_view>
#include <unordered_map>

#include <fmtlib/format.h>

#include "core/adt/vector.hpp"
#include "core/macros.hpp"

#include "config/init.hpp"

#include "option.meta.hpp"

namespace sm::config {
    REFLECT_ENUM(UpdateStatus)
    enum class UpdateStatus {
#define UPDATE_ERROR(ID, STR) ID,
#include "config.inc"
    };

    REFLECT_ENUM(OptionType)
    enum class OptionType {
#define OPTION_TYPE(ID, STR) ID,
#include "config.inc"
    };

    Group& getCommonGroup() noexcept;

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

    struct UpdateError {
        UpdateStatus error = UpdateStatus::eNone;
        std::string message;
    };

    class UpdateResult {
        std::vector<UpdateError> mErrors;

        void vfmtError(UpdateStatus status, fmt::string_view fmt, fmt::format_args args) noexcept;

    public:
        void addError(UpdateStatus status, std::string message) noexcept;

        template<typename... A>
        void fmtError(UpdateStatus status, fmt::format_string<A...> fmt, A&&... args) {
            vfmtError(status, fmt, fmt::make_format_args(args...));
        }

        std::span<const UpdateError> getErrors() const noexcept { return mErrors; }
        bool isFailure() const noexcept { return !mErrors.empty(); }
        bool isSuccess() const noexcept { return mErrors.empty(); }
        auto begin() const noexcept { return mErrors.begin(); }
        auto end() const noexcept { return mErrors.end(); }
    };

    class Group {
    public:
        constexpr Group(detail::ConfigBuilder config) noexcept
            : name(config.name)
            , description(config.description)
        { }

        constexpr Group(auto&&... args) noexcept
            : Group(detail::buildGroupKwargs<detail::ConfigBuilder>(args...))
        { }

        const std::string_view name;
        const std::string_view description;
    };

    class OptionBase {
        enum Flags : unsigned {
            eNone = 0u,

            // flag is readonly at runtime, can only be modified at startup or by code
            eReadOnly = 1u << 0,

            // flag is hidden in help messages by default
            eHidden = 1u << 1,

            // flag has been set by a configuration source
            eIsSet = 1u << 2,

            // this is an enum bitflags
            eIsEnumFlags = 1u << 3
        };

        unsigned mFlags = eNone;

        void verifyType(OptionType type) const noexcept;

    protected:
        OptionBase(detail::OptionBuilder config, OptionType type) noexcept;

        void notifySet() noexcept { mFlags |= eIsSet; }
        void notifyEnumFlags() noexcept { mFlags |= eIsEnumFlags; }

    public:
        SM_NOCOPY(OptionBase);
        SM_NOMOVE(OptionBase);

        const std::string_view name;
        const std::string_view description;
        const Group& parent;
        const OptionType type;

        bool isReadOnly() const noexcept { return mFlags & eReadOnly; }
        bool isHidden() const noexcept { return mFlags & eHidden; }

        bool isSet() const noexcept { return mFlags & eIsSet; }

        bool isEnumFlags() const noexcept { return mFlags & eIsEnumFlags; }
    };

    class Context {
        struct GroupInfo {
            std::vector<OptionBase*> options;
        };

        std::unordered_map<std::string_view, OptionBase*> mArgLookup;
        std::unordered_map<std::string_view, Group*> mGroupLookup;
        std::unordered_map<const Group*, GroupInfo> mGroups;
    public:
        const auto& groups() const noexcept { return mGroups; }

        /// @brief add a runtime generated variable to the context
        /// @warning this is a dangerous operation, the lifetime of the variable must be greater
        /// than the lifetime of the context
        /// @warning this is not thread safe
        void addToGroup(OptionBase* cvar, Group* group) noexcept;

        UpdateResult updateFromCommandLine(int argc, const char *const *argv) noexcept;
        UpdateResult updateFromConfigFile(std::istream& is) noexcept;
    };

    void addStaticVariable(Context& context, OptionBase *cvar, Group* group) noexcept;

    Context& cvars() noexcept;

    namespace detail {
        // NumericOptionValue and EnumOptionValue can only contain these types
        // this simplifies the implementation significantly.
        template<typename T>
        concept ValidNumericType
            =  std::is_same_v<T, int64_t>
            || std::is_same_v<T, uint64_t>
            || std::is_same_v<T, double>;

        template<ValidNumericType T>
        class NumericOptionValue : public OptionBase {
            std::atomic<T> mValue;
            const T mInitialValue;
            const Range<T> mRange;

        protected:
            struct Builder : detail::OptionBuilder {
                using detail::OptionBuilder::init;

                T initial = T{};
                Range<T> range {
                    .min = std::numeric_limits<T>::min(),
                    .max = std::numeric_limits<T>::max()
                };

                template<std::convertible_to<T> O>
                void init(InitialValue<O> it) noexcept { initial = it.value; }

                template<std::convertible_to<T> O>
                void init(Range<O> it) noexcept {
                    range.min = it.min;
                    range.max = it.max;
                }
            };

            NumericOptionValue(Builder config) noexcept
                : OptionBase(config, kOptionType<T>)
                , mValue(config.initial)
                , mInitialValue(config.initial)
                , mRange(config.range)
            { }

        public:
            NumericOptionValue(auto&&... args) noexcept
                : NumericOptionValue(detail::buildOptionKwargs<Builder, T>(args...))
            { }

            T getCommonValue() const noexcept { return mValue.load(); }
            T getCommonInitialValue() const noexcept { return mInitialValue; }

            Range<T> getCommonRange() const noexcept { return mRange; }

            void setCommonValue(T value) noexcept {
                mValue.store(value);
                notifySet();
            }

            void setValue(T value) noexcept {
                setCommonValue(value);
            }
        };

        template<ValidNumericType T>
        class EnumOptionValue : public OptionBase {
            std::atomic<T> mValue;

            const T mInitialValue;
            const VectorBase<EnumValue<T>> mValues;

        protected:
            struct Builder : public detail::OptionBuilder {
                using detail::OptionBuilder::init;

                T initial{};

                OptionList<T> options;
                bool bitflags = false;

                void init(EnumOptions<T> it) noexcept {
                    options = it.options;
                    bitflags = false;
                }

                void init(EnumFlags<T> it) noexcept {
                    options = it.options;
                    bitflags = true;
                }
            };

            EnumOptionValue(Builder builder) noexcept
                : OptionBase(builder, kEnumOptionType<T>)
                , mValue(builder.initial)
                , mInitialValue(builder.initial)
                , mValues(builder.options)
            {
                notifyEnumFlags();
            }
        public:
            using UnderlyingType = T;

            EnumOptionValue(auto&&... args) noexcept
                : EnumOptionValue(detail::buildOptionKwargs<Builder, T>(args...))
            { }

            T getCommonValue() const noexcept { return mValue.load(); }
            T getCommonInitialValue() const noexcept { return mInitialValue; }

            std::span<const EnumValue<T>> getCommonOptions() const noexcept {
                return mValues;
            }

            void setCommonValue(T value) noexcept {
                mValue.store(value);
                notifySet();
            }

            void setValue(T value) noexcept {
                setCommonValue(value);
            }
        };
    }

    class BoolOption : public OptionBase {
        std::atomic<bool> mValue;
        const bool mInitialValue;

    protected:
        struct Builder : detail::OptionBuilder {
            using detail::OptionBuilder::init;

            bool initial = false;

            void init(InitialValue<bool> it) noexcept { initial = it.value; }
        };

        BoolOption(Builder config) noexcept
            : OptionBase(config, OptionType::eBoolean)
            , mValue(config.initial)
            , mInitialValue(config.initial)
        { }

    public:
        BoolOption(auto&&... args) noexcept
            : BoolOption(detail::buildOptionKwargs<Builder, bool>(args...))
        { }

        bool getValue() const noexcept { return mValue.load(); }
        bool getInitialValue() const noexcept { return mInitialValue; }

        void setValue(bool value) noexcept {
            mValue.store(value);
            notifySet();
        }
    };

    class StringOption : public OptionBase {
        const std::string_view mInitialValue;

        mutable std::mutex mMutex;
        std::string mValue;

    protected:
        struct Builder : detail::OptionBuilder {
            using detail::OptionBuilder::init;

            std::string_view initial;

            template<typename O>
            void init(InitialValue<O> it) noexcept { initial = it.value; }
        };

        StringOption(Builder config) noexcept
            : OptionBase(config, OptionType::eString)
            , mInitialValue(config.initial)
            , mValue(config.initial)
        { }

    public:
        StringOption(auto&&... args) noexcept
            : StringOption(detail::buildOptionKwargs<Builder, std::string>(args...))
        { }

        // has to copy for now
        std::string getValue() const noexcept {
            std::lock_guard lock{mMutex};
            return mValue;
        }

        std::string_view getInitialValue() const noexcept { return mInitialValue; }

        void setValue(std::string_view value) noexcept {
            std::lock_guard lock{mMutex};
            mValue = value;
            notifySet();
        }
    };

    class RealOption : public detail::NumericOptionValue<double> {
        using Super = detail::NumericOptionValue<double>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    class SignedOption : public detail::NumericOptionValue<int64_t> {
        using Super = detail::NumericOptionValue<int64_t>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    class UnsignedOption : public detail::NumericOptionValue<uint64_t> {
        using Super = detail::NumericOptionValue<uint64_t>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    class SignedEnumOption : public detail::EnumOptionValue<int64_t> {
        using Super = detail::EnumOptionValue<int64_t>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    class UnsignedEnumOption : public detail::EnumOptionValue<uint64_t> {
        using Super = detail::EnumOptionValue<uint64_t>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    template<typename T>
    class Option {
        static_assert(!sizeof(T*), "unsupported type");
    };

    template<>
    class Option<bool> : public BoolOption {
        using Super = BoolOption;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    namespace detail {
        template<typename T, typename S>
        class NumericOption : public S {
            using Super = S;

        protected:
            using Builder = Super::Builder;

        public:
            using Super::Super;

            NumericOption(auto&&... args) noexcept
                : Super(detail::buildOptionKwargs<Builder, T>(Range<T>{}, args...))
            { }

            T getValue() const noexcept { return static_cast<T>(Super::getCommonValue()); }
            T getInitialValue() const noexcept { return static_cast<T>(Super::getCommonInitialValue()); }

            Range<T> getRange() const noexcept {
                auto [min, max] = Super::getCommonRange();

                return Range<T>{static_cast<T>(min), static_cast<T>(max)};
            }
        };

        template<typename T, typename S>
        class EnumOption : public S {
            using Super = S;

        protected:
            struct Builder : public Super::Builder {
                using Super::Builder::init;

                void init(InitialValue<T> it) noexcept {
                    using UnderlyingType = typename Super::UnderlyingType;
                    this->initial = static_cast<UnderlyingType>(it.value);
                }
            };

            // Builder cannot implicitly convert to Super::Builder in a way
            // that prevents overload resolution picking auto&&... instead.
            // so this explicitly casts the builder to its superclass.
            EnumOption(Builder builder) noexcept
                : Super((typename Super::Builder)builder)
            { }

        public:
            using Super::Super;

            EnumOption(auto&&... args) noexcept
                : EnumOption(detail::buildOptionKwargs<Builder, T>(args...))
            { }

            T getValue() const noexcept { return static_cast<T>(Super::getCommonValue()); }
            T getInitialValue() const noexcept { return static_cast<T>(Super::getCommonInitialValue()); }
        };
    }

    template<std::signed_integral T>
    class Option<T> : public detail::NumericOption<T, SignedOption> {
        using Super = detail::NumericOption<T, SignedOption>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    template<std::unsigned_integral T>
    class Option<T> : public detail::NumericOption<T, UnsignedOption> {
        using Super = detail::NumericOption<T, UnsignedOption>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    template<std::floating_point T>
    class Option<T> : public detail::NumericOption<T, RealOption> {
        using Super = detail::NumericOption<T, RealOption>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    template<>
    class Option<std::string> : public StringOption {
        using Super = StringOption;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    template<detail::SignedEnum T>
    class Option<T> : public detail::EnumOption<T, SignedEnumOption> {
        using Super = detail::EnumOption<T, SignedEnumOption>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    template<detail::UnsignedEnum T>
    class Option<T> : public detail::EnumOption<T, UnsignedEnumOption> {
        using Super = detail::EnumOption<T, UnsignedEnumOption>;

    protected:
        using Builder = Super::Builder;

    public:
        using Super::Super;
    };

    template<typename T>
    struct ConsoleVariable final : public config::Option<T> {
        using Super = config::Option<T>;
        using Builder = Super::Builder;

        ConsoleVariable(Builder config) noexcept
            : Super(config)
        {
            config::addStaticVariable(cvars(), this, config.group);
        }

        ConsoleVariable(auto&&... args) noexcept
            : ConsoleVariable(detail::buildOptionKwargs<Builder, T>(args...))
        { }
    };

    extern template struct ConsoleVariable<bool>;
    extern template struct ConsoleVariable<float>;
    extern template struct ConsoleVariable<double>;
    extern template struct ConsoleVariable<std::string>;
}

// NOLINTBEGIN
namespace sm {
    constinit inline config::ValueWrapper<config::InitialValue> init{};
    constinit inline config::ValueWrapper<config::Range>        range{};
    constinit inline config::ChoiceWrapper                      choices{};
    constinit inline config::FlagsWrapper                       flags{};

    constinit inline config::ConfigWrapper<config::ReadOnly,    bool>             readonly{};
    constinit inline config::ConfigWrapper<config::Hidden,      bool>             hidden{};
    constinit inline config::ConfigWrapper<config::Name,        std::string_view> name{};
    constinit inline config::ConfigWrapper<config::Description, std::string_view> desc{};
    constinit inline config::ConfigWrapper<config::Category,    config::Group&>   group{};

    template<config::detail::IsValidEnum T>
    constexpr auto val(T value) noexcept {
        using Inner = config::detail::EnumInnerType<T>;
        using Builder = config::EnumValueBuilder<Inner>;

        return Builder{static_cast<Inner>(value)};
    }

    template<typename T>
    using opt = config::ConsoleVariable<T>;
}
// NOLINTEND
