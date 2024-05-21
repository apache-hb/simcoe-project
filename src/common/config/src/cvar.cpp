#include "config/cvar.hpp"

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

static Group gCommonGroup { "general" };

Group& config::getCommonGroup() noexcept {
    return gCommonGroup;
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

    mVariables[cvar->getName()] = cvar;
    mGroups[group->getName()] = group;
}

void Context::addStaticVariable(OptionBase *cvar, Group* group) noexcept {
    CTASSERTF(isStaticStorage(group), "group %s does not have static storage duration", group->getName().data());
    CTASSERTF(isStaticStorage(cvar), "cvar %s does not have static storage duration", cvar->getName().data());
    addToGroup(cvar, group);
}

Context& config::cvars() noexcept {
    static Context instance{};
    return instance;
}

void OptionBase::verifyType(OptionType type) const noexcept {
    CTASSERTF(mType == type, "this (%s of %s) is not of type %s", mName.data(), getOptionTypeName(mType).data(), getOptionTypeName(type).data());
}

OptionBase::OptionBase(detail::OptionBuilder config, OptionType type) noexcept
    : mName(config.name)
    , mDescription(config.description)
    , mType(type)
{
    CTASSERT(type != OptionType::eUnknown);
    CTASSERT(!mName.empty());

    if (config.hidden)
        mFlags |= eHidden;

    if (config.readonly)
        mFlags |= eReadOnly;

    cvars().addToGroup(this, config.group);
}
