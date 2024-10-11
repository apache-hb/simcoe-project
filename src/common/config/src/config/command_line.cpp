#include "stdafx.hpp"
#include "common.hpp"

#include "config/config.hpp"

using namespace std::string_view_literals;

using namespace sm;
using namespace sm::config;

static void getBase(std::string_view& arg, int& base) noexcept {
    base = 10;

    if (arg.starts_with("0x") || arg.starts_with("0X")) {
        base = 16;
    } else if (arg.starts_with("0b") || arg.starts_with("0B")) {
        base = 2;
    } else if (arg.starts_with("0o") || arg.starts_with("0O")) {
        base = 8;
    }

    if (base != 10)
        arg.remove_prefix(2);
}

static void getSign(std::string_view& arg, int& sign) noexcept {
    if (arg.starts_with('-')) {
        sign = -1;
        arg.remove_prefix(1);
    }
}

static void getBaseAndSign(std::string_view& arg, int& base, int& sign) noexcept {
    getSign(arg, sign);
    getBase(arg, base);
}

static bool getFlagAction(std::string_view& flag) {
    if (flag.starts_with('-')) {
        flag.remove_prefix(1);
        return false;
    } else if (flag.starts_with('+')) {
        flag.remove_prefix(1);
    }

    return true;
}

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
        int sign = 1;

        getSign(arg, sign);

        Range range = option->getCommonRange();
        auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);

        value *= sign;

        if (ec != std::errc() || (ptr != arg.data() + arg.size())) {
            errors.fmtError(UpdateStatus::eInvalidValue, "invalid value `{}` for `{}`", arg, option->name);
        } else if (!range.contains(value)) {
            errors.fmtError(UpdateStatus::eOutOfRange, "value `{}` for `{}` is out of range [{}, {}]", value, option->name, range.min, range.max);
        } else {
            option->setCommonValue(value);
        }

        if (shouldSkip)
            ++index;
    }

    template<typename T, typename O>
    void updateInteger(UpdateResult& errors, O *option, std::string_view arg, bool shouldSkip, T value, T sign) noexcept {
        Range range = option->getCommonRange();
        auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);

        value *= sign;
        if (ec != std::errc() || (ptr != arg.data() + arg.size())) {
            errors.fmtError(UpdateStatus::eInvalidValue, "invalid value `{}` for `{}`", arg, option->name);
        } else if (!range.contains(value)) {
            errors.fmtError(UpdateStatus::eOutOfRange, "value `{}` for `{}` is out of range [{}, {}]", value, option->name, range.min, range.max);
        } else {
            option->setCommonValue(value);
        }

        if (shouldSkip)
            ++index;
    }

    void updateSigned(UpdateResult& errors, SignedOption *option, std::string_view arg, bool shouldSkip) noexcept {
        int64_t value = 0;
        int base = 10;
        int sign = 1;

        getBaseAndSign(arg, base, sign);

        updateInteger<int64_t>(errors, option, arg, shouldSkip, value, sign);
    }

    void updateUnsigned(UpdateResult& errors, UnsignedOption *option, std::string_view arg, bool shouldSkip) noexcept {
        uint64_t value = 0;
        int base = 10;

        getBase(arg, base);

        updateInteger<uint64_t>(errors, option, arg, shouldSkip, value, 1);
    }

    template<typename T>
    void updateEnum(UpdateResult& errors, T *option, std::string_view arg, bool shouldSkip) noexcept {
        std::span options = option->getCommonOptions();

        if (option->isEnumFlags()) {
            for (const auto part : std::views::split(arg, ","sv)) {
                std::string_view word{part.begin(), part.end()};
                bool shouldSetFlag = getFlagAction(word);

                if (const auto *enumValue = config::detail::getEnumValue(options, word)) {
                    auto value = option->getCommonValue();
                    sm::setMask(value, *enumValue, shouldSetFlag);
                    option->setCommonValue(value);
                } else {
                    errors.fmtError(UpdateStatus::eInvalidValue, "invalid flag `{}` for `{}`", word, option->name);
                }
            }
        } else {
            if (const auto *value = config::detail::getEnumValue(options, arg)) {
                option->setCommonValue(*value);
            } else {
                errors.fmtError(UpdateStatus::eInvalidValue, "invalid choice `{}` for `{}`", arg, option->name);
            }
        }
    }

    void updateSignedEnum(UpdateResult& errors, SignedEnumOption *option, std::string_view arg, bool shouldSkip) noexcept {
        updateEnum(errors, option, arg, shouldSkip);
    }

    void updateUnsignedEnum(UpdateResult& errors, UnsignedEnumOption *option, std::string_view arg, bool shouldSkip) noexcept {
        updateEnum(errors, option, arg, shouldSkip);
    }
};

UpdateResult Context::updateFromCommandLine(int argc, const char *const *argv) noexcept {
    if (argc <= 1)
        return UpdateResult{ };

    CommandLineParser parser = { argc, argv };
    UpdateResult errors;

    auto reportMissingValue = [&errors](std::string_view name) {
        errors.fmtError(UpdateStatus::eMissingValue, "missing value for `{}`", name);
    };

    while (parser.hasNext()) {
        std::string_view arg = parser.next();

        std::string_view name = arg;
        if (name.starts_with("--")) {
            name.remove_prefix(2);
        }
        else if (name.starts_with("/")) {
            name.remove_prefix(1);
        }
        else {
            errors.fmtError(UpdateStatus::eSyntax, "unexpected value `{}`", name);
            continue;
        }

        const char *value = nullptr;
        auto idx = name.find_first_of(":=");
        if (idx != std::string_view::npos) {
            value = name.data() + idx + 1;
            name = name.substr(0, idx);
        } else {
            value = parser.peek();
        }

        auto it = mArgLookup.find(name);

        if (it == mArgLookup.end()) {
            errors.fmtError(UpdateStatus::eUnknownOption, "unknown option `{}`", name);
            continue;
        }

        OptionBase *cvar = it->second;
        bool shouldSkip = idx == std::string_view::npos;
        if (cvar->type == OptionType::eBoolean) {
            parser.updateBoolOption(static_cast<BoolOption*>(cvar), value, shouldSkip);
            continue;
        }

        if (value == nullptr) {
            reportMissingValue(cvar->name);
            continue;
        }

        switch (cvar->type) {
        case OptionType::eString:
            parser.updateString(static_cast<StringOption*>(cvar), value, shouldSkip);
            break;

        case OptionType::eReal:
            parser.updateReal(errors, static_cast<RealOption*>(cvar), value, shouldSkip);
            break;

        case OptionType::eSigned:
            parser.updateSigned(errors, static_cast<SignedOption*>(cvar), value, shouldSkip);
            break;

        case OptionType::eUnsigned:
            parser.updateUnsigned(errors, static_cast<UnsignedOption*>(cvar), value, shouldSkip);
            break;

        case OptionType::eSignedEnum:
            parser.updateSignedEnum(errors, static_cast<SignedEnumOption*>(cvar), value, shouldSkip);
            break;

        case OptionType::eUnsignedEnum:
            parser.updateUnsignedEnum(errors, static_cast<UnsignedEnumOption*>(cvar), value, shouldSkip);
            break;

        default:
            break;
        }
    }

    return errors;
}
