#include "stdafx.hpp"

#include "config/option.hpp"

using namespace sm;
using namespace sm::config;

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
            int sign = 1;

            getSign(arg, sign);

            Range range = option->getCommonRange();
            auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);

            value *= sign;

            if (ec != std::errc() || (ptr != arg.data() + arg.size())) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eInvalidValue, fmt::format("invalid value {} for {}", arg, option->name) });
            } else if (!range.contains(value)) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eOutOfRange, fmt::format("value {} for {} is out of range [{}, {}]", value, option->name, range.min, range.max) });
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
            } else if (arg.starts_with("0") && arg.size() > 1 && std::isdigit(arg[2])) {
                base = 8;
                arg.remove_prefix(1);
            }
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

        void updateSigned(UpdateResult& errors, SignedOption *option, std::string_view arg, bool shouldSkip) noexcept {
            int64_t value = 0;
            int base = 10;
            int sign = 1;

            getBaseAndSign(arg, base, sign);

            Range range = option->getCommonRange();
            auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);

            value *= sign;
            if (ec != std::errc() || (ptr != arg.data() + arg.size())) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eInvalidValue, fmt::format("invalid value {} for {}", arg, option->name) });
            } else if (!range.contains(value)) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eOutOfRange, fmt::format("value {} for {} is out of range [{}, {}]", value, option->name, range.min, range.max) });
            } else {
                option->setCommonValue(value);
            }

            if (shouldSkip)
                ++index;
        }

        void updateUnsigned(UpdateResult& errors, UnsignedOption *option, std::string_view arg, bool shouldSkip) noexcept {
            uint64_t value = 0;
            int base = 10;

            getBase(arg, base);

            Range range = option->getCommonRange();
            auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value, base);
            if (ec != std::errc() || (ptr != arg.data() + arg.size())) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eInvalidValue, fmt::format("invalid value {} for {}", arg, option->name) });
            } else if (!range.contains(value)) {
                errors.errors.emplace_back(UpdateError{ UpdateStatus::eOutOfRange, fmt::format("value {} for {} is out of range [{}, {}]", value, option->name, range.min, range.max) });
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

        auto it = mArgLookup.find(name);

        if (it == mArgLookup.end()) {
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
