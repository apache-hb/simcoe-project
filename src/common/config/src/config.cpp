#include "stdafx.hpp"
#include "common.hpp"

#include "config/option.hpp"

using namespace sm;
using namespace sm::config;

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

    void reportInvalidType(std::string_view key, std::string_view expected, toml::node_type found) {
        result.fmtError(UpdateStatus::eInvalidValue, "invalid value for {}. expected {} found {}", key, expected, getNodeTypeString(found));
    }

    template<typename T, typename O>
    void checkedOptionUpdate(const toml::node& node, O& option, std::string_view key, toml::node_type expected) {
        if (const toml::value<T> *v = node.as<T>()) {
            config::detail::updateOption(result, option, v->get());
        } else {
            reportInvalidType(key, getNodeTypeString(expected), node.type());
        }
    }

    void updateFromTable(Group *group, std::string_view key, const toml::table& table) noexcept {
        if (group == nullptr) {
            result.fmtError(UpdateStatus::eMissingValue, "no group named {}", key);
            return;
        }

        for (const auto& [key, value] : table) {
            if (value.is_table()) {
                // TODO: if this is a flags enum then the table is handled differently
                // `bitflags = { flag1 = true, flag2 = false }`
                updateFromTable((*groupLookup)[key], key, *value.as_table());
            }
            else if (value.is_value()) {
                auto it = argLookup->find(key);
                if (it == argLookup->end()) {
                    result.fmtError(UpdateStatus::eMissingValue, "no option named {}", key.str());
                    continue;
                }

                auto& option = *it->second;

                switch (option.type) {
                case OptionType::eBoolean:
                    checkedOptionUpdate<bool>(value, (BoolOption&)option, key.str(), toml::node_type::boolean);
                    break;

                case OptionType::eString:
                    checkedOptionUpdate<std::string>(value, (StringOption&)option, key.str(), toml::node_type::string);
                    break;

                case OptionType::eReal:
                    checkedOptionUpdate<double>(value, (RealOption&)option, key.str(), toml::node_type::floating_point);
                    break;

                case OptionType::eSigned:
                    checkedOptionUpdate<int64_t>(value, (SignedOption&)option, key.str(), toml::node_type::integer);
                    break;

                case OptionType::eUnsigned:
                    checkedOptionUpdate<int64_t>(value, (UnsignedOption&)option, key.str(), toml::node_type::integer);
                    break;

                default:
                    result.fmtError(UpdateStatus::eSyntaxError, "unexpected value for option {}", key.str());
                    break;
                }
            }
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
        result.addError(UpdateStatus::eSyntaxError, std::string{parsed.error().description()});
        return result;
    }

    const toml::table& root = parsed.table();

    for (const auto& [key, value] : root) {
        if (value.is_table()) {
            source.updateFromTable(mGroupLookup[key], key, *value.as_table());
        } else {
            result.fmtError(UpdateStatus::eSyntaxError, "only expected toplevel tables {}", key.str());
        }
    }

    return result;
}
