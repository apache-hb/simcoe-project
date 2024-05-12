#pragma once

#include <concepts>

#include "core/string.hpp"
#include "core/adt/vector.hpp"

namespace sm {
    template<typename T>
    concept numeric = std::integral<T> || std::floating_point<T>;
}

namespace sm::config {
    class ConsoleVariableBase;

    class Context {
        friend ConsoleVariableBase;

        sm::Vector<ConsoleVariableBase*> mVariables;

        void add(ConsoleVariableBase* cvar) noexcept;
    };

    Context& cvars() noexcept;

    struct Name { sm::StringView value; };
    struct Description { sm::StringView value; };
    struct ReadOnly { bool readonly = true; };
    struct Hidden { bool hidden = true; };

    class ConsoleVariableBase {
        enum Flags : unsigned {
            eNone = 0,
            eReadOnly = 1 << 0,
            eHidden = 1 << 1
        };

        sm::StringView mName;
        sm::StringView mDescription;
        unsigned mFlags = eNone;

        void setDescription(sm::StringView desc) noexcept;
        void setName(sm::StringView name) noexcept;

    protected:
        ConsoleVariableBase(sm::StringView name) noexcept;

        void init(Name name) noexcept { setName(name.value); }
        void init(Description desc) noexcept { setDescription(desc.value); }
        void init(ReadOnly ro) noexcept;
        void init(Hidden hidden) noexcept;

    public:
        sm::StringView name() const noexcept { return mName; }
        sm::StringView description() const noexcept { return mDescription; }

        bool isReadOnly() const noexcept { return mFlags & eReadOnly; }
        bool isHidden() const noexcept { return mFlags & eHidden; }
    };

    template<typename T>
    struct InitialValue { T value; };

    struct InitWrapper {
        template<typename T>
        InitialValue<T> operator=(T value) const {
            return InitialValue<T>{value};
        }

        template<typename T>
        InitialValue<T> operator()(T value) const {
            return InitialValue<T>{value};
        }
    };

    template<typename T>
    struct Range { T min; T max; };

    struct RangeWrapper {
        template<typename T>
        Range<T> operator=(std::pair<T, T> value) const {
            return Range<T>{value.first, value.second};
        }

        template<typename T>
        Range<T> operator()(std::pair<T, T> value) const {
            return Range<T>{value.first, value.second};
        }
    };

    template<typename T, typename V>
    struct ConfigWrapper {
        T operator=(V value) const {
            return T{value};
        }

        T operator()(V value) const {
            return T{value};
        }
    };

    template<typename T>
    class ConsoleVariable : public ConsoleVariableBase {
        using ConsoleVariableBase::init;

        T mValue{};

        void init(InitialValue<T> init) noexcept { mValue = init.value; }

    public:
        ConsoleVariable(sm::StringView name, auto&&... args) noexcept
            : ConsoleVariableBase(name)
        {
            (this->init(args), ...);
        }
    };

    template<sm::numeric T>
    class ConsoleVariable<T> : public ConsoleVariableBase {
        using ConsoleVariableBase::init;

        T mValue{};
        Range<T> mRange{};

        void init(InitialValue<T> init) noexcept { mValue = init.value; }
        void init(Range<T> range) noexcept { mRange = range; }

    public:
        ConsoleVariable(sm::StringView name, auto&&... args) noexcept
            : ConsoleVariableBase(name)
        {
            (this->init(args), ...);
        }
    };

    template<typename T> requires (std::is_enum_v<T>)
    class ConsoleVariable<T> : public ConsoleVariableBase {
        using ConsoleVariableBase::init;

        struct Option {
            T value;
            sm::StringView name;
        };

        T mValue{};
        sm::Vector<Option> mOptions;

        void init(InitialValue<T> init) noexcept { mValue = init.value; }

    public:
        ConsoleVariable(sm::StringView name, auto&&... args) noexcept
            : ConsoleVariableBase(name)
        {
            (this->init(args), ...);
        }
    };
}

namespace sm {
    constinit config::InitWrapper init{}; // NOLINT
    constinit config::RangeWrapper range{}; // NOLINT

    constinit config::ConfigWrapper<config::ReadOnly,    bool> readonly{}; // NOLINT
    constinit config::ConfigWrapper<config::Hidden,      bool> hidden{}; // NOLINT
    constinit config::ConfigWrapper<config::Name,        sm::StringView> name{}; // NOLINT
    constinit config::ConfigWrapper<config::Description, sm::StringView> desc{}; // NOLINT

    template<typename T>
    using cvar = config::ConsoleVariable<T>; // NOLINT
}
