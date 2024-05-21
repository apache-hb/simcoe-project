#include "config/cvar.hpp"

#include <fmtlib/format.h>

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

    mVariables[cvar->name] = cvar;
    mGroups[group->name] = group;
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

UpdateResult Context::updateFromCommandLine(int argc, const char *const *argv) noexcept {
    if (argc <= 1)
        return UpdateResult{ };

    struct CommandLineParser {
        int argc;
        const char *const *argv;
        int index = 1;

        const char *next() noexcept {
            if (index < argc)
                return argv[index++];
            return nullptr;
        }

        const char *peek() const noexcept {
            if (index < argc)
                return argv[index];
            return nullptr;
        }

        bool hasNext() const noexcept {
            return index < argc;
        }

        void updateBoolOption(BoolOption *option, const char *arg, bool shouldSkip) noexcept {
            if (arg == nullptr) {
                option->setValue(true);
                return;
            }

            std::string_view value = arg;
            if (value == "true") {
                option->setValue(true);

                if (shouldSkip)
                    ++index;
            } else if (value == "false") {
                option->setValue(false);
                if (shouldSkip)
                    ++index;
            } else {
                option->setValue(true);
            }
        }

        void updateString(StringOption *option, const char *arg, bool shouldSkip) noexcept {
            option->setValue(arg);

            if (shouldSkip)
                ++index;
        }

        void updateReal(UpdateResult& errors, RealOption *option, std::string_view arg, bool shouldSkip) noexcept {
            double value = 0.0;
            auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
            if (ec != std::errc() || (ptr != arg.data() + arg.size())) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eInvalidValue, fmt::format("invalid value {} for {}", arg, option->name) });
            } else {
                option->setCommonValue(value);
            }

            if (shouldSkip)
                ++index;
        }

        static void getBase(std::string_view& arg, int& base) noexcept {
            base = 10;

            if (arg.starts_with("0x") || arg.starts_with("0X")) {
                base = 16;
                arg.remove_prefix(2);
            } else if (arg.starts_with("0b") || arg.starts_with("0B")) {
                base = 2;
                arg.remove_prefix(2);
            } else if (arg.starts_with("0") && arg.size() > 1) {
                base = 8;
                arg.remove_prefix(1);
            }
        }

        static void getBaseAndSign(std::string_view& arg, int& base, int& sign) noexcept {
            if (arg.starts_with('-')) {
                sign = -1;
                arg.remove_prefix(1);
            }

            getBase(arg, base);
        }

        void updateSigned(UpdateResult& errors, SignedOption *option, std::string_view arg, bool shouldSkip) noexcept {
            int64_t value = 0;
            int base = 10;
            int sign = 1;

            getBaseAndSign(arg, base, sign);

            auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
            if (ec != std::errc() || (ptr != arg.data() + arg.size())) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eInvalidValue, fmt::format("invalid value {} for {}", arg, option->name) });
            } else {
                option->setCommonValue(value * sign);
            }

            if (shouldSkip)
                ++index;
        }

        void updateUnsigned(UpdateResult& errors, UnsignedOption *option, std::string_view arg, bool shouldSkip) noexcept {
            uint64_t value = 0;
            int base = 10;

            getBase(arg, base);

            auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value, base);
            if (ec != std::errc() || (ptr != arg.data() + arg.size())) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eInvalidValue, fmt::format("invalid value {} for {}", arg, option->name) });
            } else {
                option->setCommonValue(value);
            }

            if (shouldSkip)
                ++index;
        }

    } parser = { argc, argv };

    UpdateResult errors;

    while (parser.hasNext()) {
        std::string_view arg = parser.next();

        std::string_view name = arg;
        if (name.starts_with("--"))
            name.remove_prefix(2);
        else if (name.starts_with("/"))
            name.remove_prefix(1);
        else
            continue;

        const char *value = nullptr;
        auto idx = name.find_first_of(":=");
        if (idx != std::string_view::npos) {
            value = name.data() + idx + 1;
            name = name.substr(0, idx);
        } else {
            value = parser.peek();
        }

        auto it = mVariables.find(name);

        if (it == mVariables.end()) {
            errors.errors.emplace_back(UpdateError{ UpdateStatus::eUnknownOption, fmt::format("unknown option {}", name) });
            continue;
        }

        OptionBase *cvar = it->second;
        switch (cvar->type) {
        case OptionType::eBoolean:
            parser.updateBoolOption(static_cast<BoolOption*>(cvar), value, idx != std::string_view::npos);
            break;

        case OptionType::eString:
            if (value == nullptr) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eMissingValue, fmt::format("missing value for {}", cvar->name) });
                continue;
            }
            parser.updateString(static_cast<StringOption*>(cvar), value, idx != std::string_view::npos);
            break;

        case OptionType::eReal:
            if (value == nullptr) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eMissingValue, fmt::format("missing value for {}", cvar->name) });
                continue;
            }
            parser.updateReal(errors, static_cast<RealOption*>(cvar), value, idx != std::string_view::npos);
            break;

        case OptionType::eSigned:
            if (value == nullptr) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eMissingValue, fmt::format("missing value for {}", cvar->name) });
                continue;
            }
            parser.updateSigned(errors, static_cast<SignedOption*>(cvar), value, idx != std::string_view::npos);
            break;

        case OptionType::eUnsigned:
            if (value == nullptr) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eMissingValue, fmt::format("missing value for {}", cvar->name) });
                continue;
            }
            parser.updateUnsigned(errors, static_cast<UnsignedOption*>(cvar), value, idx != std::string_view::npos);
            break;

        default:
            break;
        }
    }

    return UpdateResult{ errors };
}
