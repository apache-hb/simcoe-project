#pragma once

#include <map>
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
        std::string_view mName;
        sm::Vector<OptionBase*> mVariables;

    public:
        Group(std::string_view name) noexcept
            : mName(name)
        { }

        void add(OptionBase* cvar) noexcept;

        std::string_view getName() const noexcept { return mName; }

        auto getVariables(this auto& self) noexcept { return std::span(self.mVariables); }
    };

    Group& getCommonGroup() noexcept;

    namespace detail {
        template<typename T>
        constexpr inline OptionType kOptionType = OptionType::eUnknown;

        template<std::integral T>
        constexpr inline OptionType kOptionType<T> = OptionType::eInteger;

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
        constexpr T buildOptionKwargs(auto&&... args) {
            T builder{};
            builder.type = kOptionType<V>;

            (builder.init(std::forward<decltype(args)>(args)), ...);
            return builder;
        }
    }

    class Context {
    public:
        std::map<std::string_view, OptionBase*> mVariables;
        std::map<std::string_view, Group*> mGroups;

        std::unordered_map<std::string_view, OptionBase*> mNameLookup;

        void addToGroup(OptionBase* cvar, Group* group) noexcept;
        void addStaticVariable(OptionBase *cvar, Group* group) noexcept;

        void updateFromCommandLine(int argc, const char** argv) noexcept;
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

        std::string_view mName;
        std::string_view mDescription;
        unsigned mFlags = eNone;
        OptionType mType;

        void verifyType(OptionType type) const noexcept;

    protected:
        OptionBase(detail::OptionBuilder config, OptionType type) noexcept;

    public:
        std::string_view getName() const noexcept { return mName; }
        std::string_view getDescription() const noexcept { return mDescription; }
        OptionType getType() const noexcept { return mType; }

        bool isReadOnly() const noexcept { return mFlags & eReadOnly; }
        bool isHidden() const noexcept { return mFlags & eHidden; }

        bool isSet() const noexcept { return mFlags & eIsSet; }

        SM_NOCOPY(OptionBase);
        SM_NOMOVE(OptionBase);
    };

    template<typename T> requires (detail::kOptionType<T> != OptionType::eUnknown)
    class OptionWithValue : public OptionBase {
        T mValue;
        T mInitialValue;

    protected:
        struct Builder : detail::OptionBuilder {
            using detail::OptionBuilder::init;

            T initial{};

            template<typename O>
            void init(InitialValue<O> it) noexcept {
                initial = T{it.value};
            }
        };

        OptionWithValue(Builder config) noexcept
            : OptionBase(config, config.type)
            , mValue(config.initial)
            , mInitialValue(config.initial)
        { }

    public:
        OptionWithValue(auto&&... args) noexcept
            : OptionWithValue(detail::buildOptionKwargs<Builder, T>(args...))
        { }

        T getValue() const noexcept { return mValue; }
        T getInitialValue() const noexcept { return mInitialValue; }
    };

    template<typename T>
    class Option : public OptionWithValue<T> {
        using Super = OptionWithValue<T>;

    public:
        using Super::Super;
    };

    template<sm::numeric T>
    class Option<T> : public OptionWithValue<T> {
        using Super = OptionWithValue<T>;

        Range<T> mRange;

    protected:
        struct Builder : Super::Builder {
            using Super::Builder::init;

            Range<T> range {
                .min = std::numeric_limits<T>::min(),
                .max = std::numeric_limits<T>::max()
            };

            void init(Range<T> it) noexcept { range = it; }
        };

    public:
        Option(auto&&... args) noexcept
            : Option(detail::buildOptionKwargs<Builder, T>(args...))
        { }

        Option(Builder config) noexcept
            : Super((typename Super::Builder)config)
            , mRange(config.range)
        { }

        Range<T> getRange() const noexcept { return mRange; }
    };

    template<typename T> requires (std::is_enum_v<T> || std::is_scoped_enum_v<T>)
    class Option<T> : public OptionWithValue<T> {
        using Super = OptionWithValue<T>;

        OptionList<T> mOptions;

    protected:
        struct Builder : Super::Builder {
            using Super::Builder::init;

            OptionList<T> options;

            void init(EnumOptions<T> it) noexcept {
                Super::Builder::type = OptionType::eEnum;
                options = it.options;
            }

            void init(EnumFlags<T> it) noexcept {
                Super::Builder::type = OptionType::eFlags;
                options = it.options;
            }
        };

        Option(Builder config) noexcept
            : Super((typename Super::Builder)config)
            , mOptions(config.options)
        { }

    public:
        Option(auto&&... args) noexcept
            : Option(detail::buildOptionKwargs<Builder, T>(args...))
        { }

        std::span<const EnumValue<T>> getOptions() const noexcept { return mOptions; }
    };

    template<typename T>
    struct ConsoleVariable : public config::Option<T> {
        using Super = config::Option<T>;
        using Builder = Super::Builder;

        ConsoleVariable(Builder config) noexcept
            : Super(config)
        {
            cvars().addToGroup(this, config.group);
        }

        ConsoleVariable(auto&&... args) noexcept
            : ConsoleVariable(detail::buildOptionKwargs<Builder, T>(args...))
        { }
    };
}

// NOLINTBEGIN
namespace sm {
    constinit inline config::ValueWrapper<config::InitialValue> init{};
    constinit inline config::ValueWrapper<config::Range>        range{};
    constinit inline config::OptionWrapper  options{};
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
