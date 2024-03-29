#pragma once

#include <concepts>

#include "core/string.hpp"
#include "core/vector.hpp"

namespace sm {
    template<typename T>
    concept numeric = std::integral<T> || std::floating_point<T>;
}

namespace sm::config {
    class ConsoleVariableBase;
    class ConsoleVariableGroup;

    class Context {
        friend ConsoleVariableBase;

        sm::Vector<ConsoleVariableBase*> mVariables;
        sm::Vector<ConsoleVariableGroup> mGroups;

        void add(ConsoleVariableBase* cvar);
    };

    Context& cvars();

    class ConsoleVariableGroup {

    };

    struct Description { sm::StringView value; };
    struct ReadOnly { bool readonly = true; };

    class ConsoleVariableBase {
        enum Flags : unsigned {
            eNone = 0,
            eReadOnly = 1 << 0,
        };

        sm::StringView mName;
        sm::StringView mDescription;
        unsigned mFlags = eNone;

        virtual bool parse(sm::StringView value) = 0;

    protected:
        ConsoleVariableBase(sm::StringView name);

        void set_description(sm::StringView desc);

        void init(Description desc);
        void init(ReadOnly ro);

    public:
        sm::StringView name() const { return mName; }
        sm::StringView description() const { return mDescription; }
    };

    template<typename T>
    struct InitialValue { T value; };

    template<typename T>
    struct Range { T min; T max; };

    template<typename T>
    class ConsoleVariable : public ConsoleVariableBase {
        T mValue{};

        void init(InitialValue<T> init) { mValue = init.value; }
        void init(Description desc) { set_description(desc.value); }

    public:
        ConsoleVariable(sm::StringView name, auto&&... args)
            : ConsoleVariableBase(name)
        {
            (init(args), ...);
        }
    };

    template<sm::numeric T>
    class ConsoleVariable<T> : public ConsoleVariableBase {
        T mValue{};
        Range<T> mRange{};

        void init(InitialValue<T> init) { mValue = init.value; }
        void init(Range<T> range) { mRange = range; }
        void init(Description desc) { set_description(desc.value); }

    public:
        ConsoleVariable(sm::StringView name, auto&&... args)
            : ConsoleVariableBase(name)
        {
            (init(args), ...);
        }
    };
}

namespace sm {
    template<typename T>
    using init = config::InitialValue<T>; // NOLINT

    template<typename T>
    using range = config::Range<T>; // NOLINT

    constinit auto readonly = [] { // NOLINT
        struct Wrapper : config::ReadOnly {
            constexpr config::ReadOnly operator=(bool value) const {
                return config::ReadOnly{value};
            }

            constexpr config::ReadOnly operator()(bool value) const {
                return config::ReadOnly{value};
            }
        };

        return Wrapper{};
    }();

    constinit auto desc = [] { // NOLINT
        struct Wrapper {
            constexpr config::Description operator=(sm::StringView value) const {
                return config::Description{value};
            }

            constexpr config::Description operator()(sm::StringView value) const {
                return config::Description{value};
            }
        };

        return Wrapper{};
    }();

    using group = config::ConsoleVariableGroup; // NOLINT

    template<typename T>
    using cvar = config::ConsoleVariable<T>; // NOLINT
}
