#include "stdafx.hpp"

#include "config/option.hpp"

using namespace sm;
using namespace sm::config;

static sm::opt<bool> gOptionHelp {
    name = "help",
    desc = "print this message"
};

static sm::opt<bool> gOptionVersion {
    name = "version",
    desc = "print version information"
};

#if 0
enum class Choice {
    eFirst, eSecond, eThird
};

static sm::opt<Choice> gChoiceFlag {
    name = "choice of flags",
    desc = "choose one of the flags",
    options = {
        val(Choice::eFirst) = "first",
        val(Choice::eSecond) = "second",
        val(Choice::eThird) = "third"
    }
};
#endif


Group& config::getCommonGroup() noexcept {
    static Group instance { name = "general" };
    return instance;
}

constexpr std::string_view getOptionTypeName(OptionType type) noexcept {
    switch (type) {
#define OPTION_TYPE(ID, STR) case OptionType::ID: return STR;
#include "config/config.inc"
    default: return "unknown";
    }
}

void Context::addToGroup(OptionBase *cvar, Group* group) noexcept {
    CTASSERT(cvar != nullptr);
    CTASSERT(group != nullptr);

    mVariables[cvar->name] = cvar;
    mGroups[group->name] = group;

    mArgLookup[cvar->name] = cvar;
}

void Context::addStaticVariable(OptionBase *cvar, Group* group) noexcept {
    CTASSERTF(isStaticStorage(group), "group %s does not have static storage duration", group->name.data());
    CTASSERTF(isStaticStorage(cvar), "cvar %s does not have static storage duration", cvar->name.data());
    addToGroup(cvar, group);
}

Context& config::cvars() noexcept {
    static Context instance{};
    return instance;
}

void OptionBase::verifyType(OptionType other) const noexcept {
    CTASSERTF(type == other, "this (%s of %s) is not of type %s", name.data(), getOptionTypeName(type).data(), getOptionTypeName(other).data());
}

OptionBase::OptionBase(detail::OptionBuilder config, OptionType type) noexcept
    : name(config.name)
    , description(config.description)
    , type(type)
{
    CTASSERT(type != OptionType::eUnknown);
    CTASSERT(!name.empty());

    if (config.hidden)
        mFlags |= eHidden;

    if (config.readonly)
        mFlags |= eReadOnly;

    cvars().addToGroup(this, config.group);
}

template struct sm::config::ConsoleVariable<bool>;
template struct sm::config::ConsoleVariable<int>;
template struct sm::config::ConsoleVariable<unsigned>;
template struct sm::config::ConsoleVariable<float>;
template struct sm::config::ConsoleVariable<double>;
template struct sm::config::ConsoleVariable<std::string>;
