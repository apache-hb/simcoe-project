#include "stdafx.hpp"
#include "core/core.hpp"
#include "core/error.hpp"

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

Group& config::getCommonGroup() noexcept {
    static Group instance = [] {
        detail::ConfigBuilder config {
            .name = "common",
            .group = nullptr,
        };
        return Group{config};
    }();

    return instance;
}

void UpdateResult::vfmtError(UpdateStatus status, fmt::string_view fmt, fmt::format_args args) noexcept {
    addError(status, fmt::vformat(fmt, args));
}

void UpdateResult::addError(UpdateStatus status, std::string message) noexcept {
    mErrors.emplace_back(UpdateError{status, std::move(message)});
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

#if SMC_DEBUG
    SM_ASSERTF(mArgLookup.find(cvar->name) == mArgLookup.end(), "cvar {} already exists", cvar->name);
#endif

    mGroups[group].options.push_back(cvar);
    mGroups[group->parent].children.push_back(group);

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
    SM_ASSERTF(type == otherType, "this ({} of {}) is not of type {}", name, getOptionTypeName(type), getOptionTypeName(otherType));
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
template struct sm::config::ConsoleVariable<float>;
template struct sm::config::ConsoleVariable<double>;
template struct sm::config::ConsoleVariable<std::string>;
