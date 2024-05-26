#include "stdafx.hpp"
#include "core/core.hpp"

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

    // be very careful to not use group->parent aside from taking its address
    // its lifetime may not have started yet

    mGroups[group].options.emplace(cvar->name, cvar);
    mGroups[&group->parent].children.emplace(group->name, group);

    mArgLookup[cvar->name] = cvar;
    mGroupLookup[group->name] = group;
}

void config::addStaticVariable(Context& context, OptionBase *cvar, Group* group) noexcept {
    CTASSERTF(isStaticStorage(group), "group %s does not have static storage duration", group->name.data());
    CTASSERTF(isStaticStorage(cvar), "cvar %s does not have static storage duration", cvar->name.data());
    context.addToGroup(cvar, group);
}

Context& config::cvars() noexcept {
    static Context instance{};
    return instance;
}

void OptionBase::verifyType(OptionType otherType) const noexcept {
    CTASSERTF(type == otherType, "this (%s of %s) is not of type %s", name.data(), getOptionTypeName(type).data(), getOptionTypeName(otherType).data());
}

OptionBase::OptionBase(detail::OptionBuilder config, OptionType type) noexcept
    : name(config.name)
    , description(config.description)
    , parent(*config.group)
    , type(type)
{
    CTASSERT(type != OptionType::eUnknown);
    CTASSERT(!name.empty());

    if (config.hidden)
        mFlags |= eHidden;

    if (config.readonly)
        mFlags |= eReadOnly;
}

template struct sm::config::ConsoleVariable<bool>;
template struct sm::config::ConsoleVariable<int>;
template struct sm::config::ConsoleVariable<unsigned>;
template struct sm::config::ConsoleVariable<float>;
template struct sm::config::ConsoleVariable<double>;
template struct sm::config::ConsoleVariable<std::string>;
