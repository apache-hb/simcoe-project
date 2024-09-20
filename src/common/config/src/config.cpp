#include "stdafx.hpp"
#include "common.hpp"

#include "core/error.hpp"
#include "config/config.hpp"

using namespace sm;
using namespace sm::config;

bool config::detail::verifyWriteAccess(UpdateResult& errs, const OptionBase& option) {
    if (option.isReadOnly()) {
        errs.fmtError(UpdateStatus::eReadOnly, "option {} is read only", option.name);
        return false;
    }

    return true;
}

static std::string_view getNodeTypeString(toml::node_type type) noexcept {
    using enum toml::node_type;
    switch (type) {
    case none: return "none";
    case table: return "table";
    case array: return "array";
    case string: return "string";
    case integer: return "integer";
    case floating_point: return "floating point";
    case boolean: return "boolean";
    case date: return "date";
    case time: return "time";
    case date_time: return "date time";
    default: return "unknown";
    }
}

struct ConfigFileSource {
    UpdateResult& result;
    std::unordered_map<std::string_view, OptionBase*> *argLookup;
    std::unordered_map<std::string_view, Group*> *groupLookup;

    void reportInvalidType(std::string_view key, std::string_view expected, toml::node_type found) noexcept {
        result.fmtError(UpdateStatus::eInvalidValue, "invalid value for {}. expected {} found {}", key, expected, getNodeTypeString(found));
    }

    template<typename T, typename O>
    void checkedOptionUpdate(const toml::node& node, O& option, std::string_view key, toml::node_type expected) noexcept {
        if (const toml::value<T> *v = node.as<T>()) {
            config::detail::updateOption(result, option, v->get());
        } else {
            reportInvalidType(key, getNodeTypeString(expected), node.type());
        }
    }

    template<typename O>
    void updateEnumFlagOption(O &option, std::string_view key, const toml::table& table) noexcept {
        for (const auto& [key, value] : table) {
            if (const toml::value<bool> *asBool = value.as_boolean()) {
                const auto *it = config::detail::getEnumValue(option.getCommonOptions(), key);
                if (it == nullptr) {
                    result.fmtError(UpdateStatus::eInvalidValue, "invalid flag `{}` for `{}`", key.str(), option.name);
                    continue;
                }

                auto optionValue = option.getCommonValue();
                sm::setMask(optionValue, *it, asBool->get());
                option.setCommonValue(optionValue);
            } else {
                reportInvalidType(key, "boolean", value.type());
            }
        }
    }

    template<typename O>
    void updateEnumOption(O &option, std::string_view key, const toml::node& node) noexcept {
        if (const toml::value<std::string> *asString = node.as_string()) {
            const auto *it = config::detail::getEnumValue(option.getCommonOptions(), asString->get());
            if (it == nullptr) {
                result.fmtError(UpdateStatus::eInvalidValue, "invalid choice `{}` for `{}`", asString->get(), option.name);
                return;
            }

            option.setCommonValue(*it);
        } else {
            reportInvalidType(key, "string", node.type());
        }
    }

    bool tryUpdateFlags(std::string_view key, const toml::table& table) noexcept {
        auto it = argLookup->find(key);
        if (it == argLookup->end()) {
            return false;
        }

        auto& option = *it->second;

        if (!option.isEnumFlags()) {
            result.fmtError(UpdateStatus::eSyntax, "unexpected table for option {}", key);
            return true;
        }

        switch (option.type) {
        case OptionType::eSignedEnum:
            updateEnumFlagOption((SignedEnumOption&)option, key, table);
            break;
        case OptionType::eUnsignedEnum:
            updateEnumFlagOption((UnsignedEnumOption&)option, key, table);
            break;
        default:
            SM_NEVER("unexpected enum flag option type {}", key);
            break;
        }

        return true;
    }

    void updateValue(Group *group, std::string_view key, const toml::node& node) noexcept {
        if (node.is_table()) {
            const toml::table& table = *node.as_table();
            if (!tryUpdateFlags(key, table)) {
                updateFromTable((*groupLookup)[key], key, table);
            }
        }
        else if (node.is_value()) {
            auto it = argLookup->find(key);
            if (it == argLookup->end()) {
                result.fmtError(UpdateStatus::eMissingValue, "no option named {}", key);
                return;
            }

            auto& option = *it->second;

            switch (option.type) {
            case OptionType::eBoolean:
                checkedOptionUpdate<bool>(node, (BoolOption&)option, key, toml::node_type::boolean);
                break;

            case OptionType::eString:
                checkedOptionUpdate<std::string>(node, (StringOption&)option, key, toml::node_type::string);
                break;

            case OptionType::eReal:
                checkedOptionUpdate<double>(node, (RealOption&)option, key, toml::node_type::floating_point);
                break;

            case OptionType::eSigned:
                checkedOptionUpdate<int64_t>(node, (SignedOption&)option, key, toml::node_type::integer);
                break;

            case OptionType::eUnsigned:
                checkedOptionUpdate<int64_t>(node, (UnsignedOption&)option, key, toml::node_type::integer);
                break;

            case OptionType::eSignedEnum:
                updateEnumOption((SignedEnumOption&)option, key, node);
                break;

            case OptionType::eUnsignedEnum:
                updateEnumOption((UnsignedEnumOption&)option, key, node);
                break;

            default:
                result.fmtError(UpdateStatus::eSyntax, "unexpected value for option {}", key);
                break;
            }
        }
    }

    void updateFromTable(Group *group, std::string_view key, const toml::table& table) noexcept {
        if (group == nullptr) {
            result.fmtError(UpdateStatus::eMissingValue, "no group named {}", key);
            return;
        }

        for (const auto& [key, value] : table) {
            updateValue(group, key, value);
        }
    }
};

UpdateResult Context::updateFromConfigFile(std::istream& is) noexcept {
    UpdateResult result;
    ConfigFileSource source {
        .result = result,
        .argLookup = &mArgLookup,
        .groupLookup = &mGroupLookup
    };

    toml::parse_result parsed = toml::parse(is);

    if (parsed.failed()) {
        result.addError(UpdateStatus::eSyntax, std::string{parsed.error().description()});
        return result;
    }

    const toml::table& root = parsed.table();

    for (const auto& [key, value] : root) {
        if (value.is_table()) {
            // if this is a flag option then we update the option rather than treating it as a group
            if (!source.tryUpdateFlags(key, *value.as_table())) {
                source.updateFromTable(mGroupLookup[key], key, *value.as_table());
            }
        } else {
            source.updateValue(&getCommonGroup(), key, value);
        }
    }

    return result;
}
