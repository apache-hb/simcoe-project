#include "config/cvar.hpp"

using namespace sm;
using namespace sm::config;

void Context::add(ConsoleVariableBase* cvar) {
    mVariables.push_back(cvar);
}

config::Context& config::cvars() {
    static Context instance{};
    return instance;
}

ConsoleVariableBase::ConsoleVariableBase(sm::StringView name)
    : mName(name)
{
    // promise that these will always have static storage duration
    cvars().add(this);
}

void ConsoleVariableBase::set_description(sm::StringView description) {
    mDescription = description;
}

void ConsoleVariableBase::init(Description desc) {
    set_description(desc.value);
}

void ConsoleVariableBase::init(ReadOnly ro) {
    if (ro.readonly)
        mFlags |= eReadOnly;
    else
        mFlags &= ~eReadOnly;
}
