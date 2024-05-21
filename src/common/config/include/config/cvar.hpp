#pragma once

#include <map>
#include <mutex>
#include <span>
#include <string_view>
#include <unordered_map>

#include "core/macros.hpp"
#include "core/adt/vector.hpp"

#include "config/init.hpp"

namespace sm::config {
    class OptionBase;
    class Group;

    enum class OptionType {
#define OPTION_TYPE(ID, STR) ID,
#include "config.inc"
    };

    class Group {
        sm::Vector<OptionBase*> mVariables;

    public:
        Group(std::string_view name) noexcept
            : name(name)
        { }

        const std::string_view name;

        void add(OptionBase* cvar) noexcept;

        auto getVariables(this auto& self) noexcept { return std::span(self.mVariables); }
    };

    Group& getCommonGroup() noexcept;

    namespace detail {
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

        template<typename T> requires (std::is_enum_v<T> || std::is_scoped_enum_v<T>)
        constexpr inline OptionType kOptionType<T> = OptionType::eEnum;

        struct OptionBuilder {
            std::string_view name;
            std::string_view description = "no description";
            bool readonly = false;
            bool hidden = false;

            Group* group = &getCommonGroup();
            OptionType type;

            void init(Name it) noexcept { name = it.value; }
            void init(Description it) noexcept { description = it.value; }
            void init(ReadOnly it) noexcept { readonly = it.readonly; }
            void init(Hidden it) noexcept { hidden = it.hidden; }
            void init(Category it) noexcept { group = &it.group; }

            OptionBuilder() noexcept = default;
        };

        template<std::derived_from<OptionBuilder> T, typename V> requires (kOptionType<V> != OptionType::eUnknown)
        constexpr T buildOptionKwargs(auto&&... args) noexcept {
            T builder{};
            builder.type = kOptionType<V>;

            (builder.init(args), ...);
            return builder;
        }
    }

    enum class UpdateStatus {
#define UPDATE_ERROR(ID, STR) ID,
#include "config.inc"
    };

    constexpr std::string_view toString(UpdateStatus error) noexcept {
        switch (error) {
#define UPDATE_ERROR(ID, STR) case UpdateStatus::ID: return STR;
#include "config.inc"
        default: return "unknown";
        }
    }

    struct UpdateError {
        UpdateStatus error = UpdateStatus::eNone;
        std::string message;
    };

    struct UpdateResult {
        std::vector<UpdateError> errors;
    };

    class Context {
    public:
        std::map<std::string_view, OptionBase*> mVariables;
        std::map<std::string_view, Group*> mGroups;

        std::unordered_map<std::string_view, OptionBase*> mNameLookup;

        /// @brief add a runtime generated variable to the context
        /// @warning this is a dangerous operation, the lifetime of the variable must be greater
        /// than the lifetime of the context
        /// @warning this is not thread safe
        void addToGroup(OptionBase* cvar, Group* group) noexcept;

        /// @brief add a static variable to the context
        void addStaticVariable(OptionBase *cvar, Group* group) noexcept;

        UpdateResult updateFromCommandLine(int argc, const char *const *argv) noexcept;
        void updateFromConfigFile(std::istream& is) noexcept;
        void updateFromEnvironment() noexcept;
    };

    Context& cvars() noexcept;

    class OptionBase {
        enum Flags : unsigned {
            eNone = 0u,

            // flag is readonly at runtime, can only be modified at startup or by code
            eReadOnly = 1u << 0,

            // flag is hidden in help messages by default
            eHidden = 1u << 1,

            // flag has been set by a configuration source
            eIsSet = 1u << 2
        };

        unsigned mFlags = eNone;

        void verifyType(OptionType type) const noexcept;

    protected:
        OptionBase(detail::OptionBuilder config, OptionType type) noexcept;

        void notifySet() noexcept { mFlags |= eIsSet; }

    public:
        SM_NOCOPY(OptionBase);
        SM_NOMOVE(OptionBase);

        const std::string_view name;
        const std::string_view description;
        const OptionType type;

        bool isReadOnly() const noexcept { return mFlags & eReadOnly; }
        bool isHidden() const noexcept { return mFlags & eHidden; }

        bool isSet() const noexcept { return mFlags & eIsSet; }
    };

    namespace detail {
        template<typename T>
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
                : S(detail::buildOptionKwargs<Builder, T>(Range<T>{}, args...))
            { }

            T getValue() const noexcept { return static_cast<T>(Super::getCommonValue()); }
            T getInitialValue() const noexcept { return static_cast<T>(Super::getCommonInitialValue()); }

            Range<T> getRange() const noexcept {
                auto [min, max] = Super::getCommonRange();

                return Range<T>{static_cast<T>(min), static_cast<T>(max)};
            }
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

    template<typename T>
    struct ConsoleVariable : public config::Option<T> {
        using Super = config::Option<T>;
        using Builder = Super::Builder;

        ConsoleVariable(Builder config) noexcept
            : Super(config)
        {
            cvars().addStaticVariable(this, config.group);
        }

        ConsoleVariable(auto&&... args) noexcept
            : ConsoleVariable(detail::buildOptionKwargs<Builder, T>(args...))
        { }
    };

    extern template struct ConsoleVariable<bool>;
    extern template struct ConsoleVariable<int>;
    extern template struct ConsoleVariable<unsigned>;
    extern template struct ConsoleVariable<float>;
    extern template struct ConsoleVariable<double>;
    extern template struct ConsoleVariable<std::string>;
}

// NOLINTBEGIN
namespace sm {
    constinit inline config::ValueWrapper<config::InitialValue> init{};
    constinit inline config::ValueWrapper<config::Range>        range{};
    constinit inline config::OptionWrapper                      options{};
    constinit inline config::ValueWrapper<config::EnumFlags>    flags{};

    constinit inline config::ConfigWrapper<config::ReadOnly,    bool>             readonly{};
    constinit inline config::ConfigWrapper<config::Hidden,      bool>             hidden{};
    constinit inline config::ConfigWrapper<config::Name,        std::string_view> name{};
    constinit inline config::ConfigWrapper<config::Description, std::string_view> desc{};
    constinit inline config::ConfigWrapper<config::Category,    config::Group&>   group{};

    template<typename T>
    using opt = config::ConsoleVariable<T>;
}
// NOLINTEND
