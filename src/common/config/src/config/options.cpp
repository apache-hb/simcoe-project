#include "stdafx.hpp"

#include "core/core.hpp"
#include "core/error.hpp"
#include "config/config.hpp"
#include "config/parse.hpp"

#include "config/config.hpp"

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


std::string_view config::toString(OptionType it) {
    switch (it) {
    case OptionType::eUnknown: return "UNKNOWN";
    case OptionType::eBoolean: return "BOOLEAN";
    case OptionType::eSigned: return "SIGNED";
    case OptionType::eUnsigned: return "UNSIGNED";
    case OptionType::eReal: return "REAL";
    case OptionType::eString: return "STRING";
    case OptionType::eSignedEnum:
    case OptionType::eUnsignedEnum:
        return "ENUM";

    default: return "INVALID";
    }
}

std::string_view config::toString(UpdateStatus it) {
    switch (it) {
    case UpdateStatus::eSuccess: return "SUCCESS";
    case UpdateStatus::eUnknownOption: return "UNKNOWN OPTION";
    case UpdateStatus::eMissingValue: return "MISSING VALUE";
    case UpdateStatus::eInvalidValue: return "INVALID VALUE";
    case UpdateStatus::eOutOfRange: return "OUT OF RANGE";
    case UpdateStatus::eReadOnly: return "READ ONLY";
    case UpdateStatus::eSyntax: return "INVALID SYNTAX";
    default: return "INVALID";
    }
}

const Group& config::getCommonGroup() noexcept {
    static Group sInstance = [] {
        detail::ConfigBuilder config {
            .name = "common",
            .description = "common options",
        };
        return Group{config};
    }();

    return sInstance;
}

void UpdateResult::vfmtError(UpdateStatus status, fmt::string_view fmt, fmt::format_args args) {
    addError(status, fmt::vformat(fmt, args));
}

void UpdateResult::addError(UpdateStatus status, std::string message) {
    mErrors.emplace_back(UpdateError{status, std::move(message)});
}

constexpr std::string_view getOptionTypeName(OptionType type) noexcept {
    return toString(type);
}

void Context::addToGroup(OptionBase *cvar, const Group* group) noexcept {
    CTASSERT(cvar != nullptr);
    CTASSERT(group != nullptr);

    // be very careful to not use group->parent aside from taking its address
    // its lifetime may not have started yet

#if SMC_DEBUG
    SM_ASSERTF(mArgLookup.find(cvar->name) == mArgLookup.end(), "cvar {} already exists", cvar->name);
#endif

    mGroups[group].options.push_back(cvar);

    mArgLookup[cvar->name] = cvar;
    mGroupLookup[group->name] = group;
}

void config::addStaticVariable(Context& context, OptionBase *cvar, const Group* group) noexcept {
    CTASSERTF(isStaticStorage(group, true), "group %s does not have static storage duration", group->name.data());
    CTASSERTF(isStaticStorage(cvar, true), "cvar %s does not have static storage duration", cvar->name.data());
    context.addToGroup(cvar, group);
}

Context& config::cvars() noexcept {
    static Context sContext{};
    return sContext;
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

static void printOption(const OptionBase& option) noexcept {
    if (option.isHidden())
        return;

    fmt::println("  --{}: {}", option.name, option.description);
}

int sm::parseCommandLine(int argc, const char **argv) noexcept {
    auto& ctx = sm::config::cvars();
    auto result = ctx.updateFromCommandLine(argc, argv);
    for (const auto& msg : result.getErrors()) {
        fmt::println("{}: {}", toString(msg.error), msg.message);
    }

    if (result.isFailure())
        return 1;

    if (gOptionHelp.getValue()) {
        const auto& allGroups = ctx.groups();
        auto groups = [&] {
            std::vector<const Group*> result;
            result.reserve(allGroups.size());

            for (const auto& [group, data] : allGroups) {
                result.push_back(group);
            }

            std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                return a->name < b->name;
            });

            return result;
        }();

        for (const Group *group : groups) {
            fmt::println("{} - {}", group->name, group->description);
            for (const auto& option : allGroups.at(group).options) {
                printOption(*option);
            }
        }

        return -1;
    }

    return 0;
}
