#include "config/cvar.hpp"

using namespace sm;
using namespace sm::config;

void Context::add(ConsoleVariableBase* cvar) noexcept {
    mVariables.push_back(cvar);
}

config::Context& config::cvars() noexcept {
    static Context instance{};
    return instance;
}

ConsoleVariableBase::ConsoleVariableBase(sm::StringView name) noexcept
    : mName(name)
{
    // promise that these will always have static storage duration
    cvars().add(this);
}

void ConsoleVariableBase::setDescription(sm::StringView description) noexcept {
    mDescription = description;
}

void ConsoleVariableBase::setName(sm::StringView name) noexcept {
    mName = name;
}

void ConsoleVariableBase::init(ReadOnly ro) noexcept {
    if (ro.readonly)
        mFlags |= eReadOnly;
    else
        mFlags &= ~eReadOnly;
}

void ConsoleVariableBase::init(Hidden hidden) noexcept {
    if (hidden.hidden)
        mFlags |= eHidden;
    else
        mFlags &= ~eHidden;
}
