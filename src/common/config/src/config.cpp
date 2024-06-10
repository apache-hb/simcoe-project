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

UpdateResult Context::updateFromConfigFile(std::istream& is) noexcept {
    toml::parse_result parsed = toml::parse(is);
    UpdateResult result;
    if (parsed.failed()) {
        result.addError(UpdateStatus::eSyntaxError, std::string{parsed.error().description()});
        return result;
    }

    auto reportInvalidType = [&](std::string_view key, std::string_view expected, toml::node_type found) {
        result.addError(
            UpdateStatus::eInvalidValue,
            fmt::format("invalid value for {}. expected {} found {}", key, expected, getNodeTypeString(found))
        );
    };

    const toml::table& root = parsed.table();

    auto acceptTable = [&](this auto& self, Group *group, std::string_view key, const toml::table& table) {
        if (group == nullptr) {
            result.addError(
                UpdateStatus::eMissingValue,
                fmt::format("no group named {}", key)
            );
            return;
        }

        for (const auto& [key, value] : table) {
            if (value.is_table()) {
                self(mGroupLookup[key], key, *value.as_table());
            }
            else if (value.is_value()) {
                auto it = mArgLookup.find(key);
                if (it == mArgLookup.end()) {
                    result.addError(
                        UpdateStatus::eMissingValue,
                        fmt::format("no option named {}", key.str())
                    );
                    continue;
                }

                auto& option = *it->second;
                if (option.type == OptionType::eBoolean) {
                    if (const toml::value<bool> *v = value.as_boolean()) {
                        detail::updateOption(result, (BoolOption&)option, v->get());
                    } else {
                        reportInvalidType(key.str(), "boolean", value.type());
                    }
                }
                else if (option.type == OptionType::eString) {
                    if (const toml::value<std::string> *v = value.as_string()) {
                        detail::updateOption(result, (StringOption&)option, v->get());
                    } else {
                        reportInvalidType(key.str(), "string", value.type());
                    }
                }
                else if (option.type == OptionType::eReal) {
                    if (const toml::value<double> *v = value.as_floating_point()) {
                        detail::updateNumericOption(result, (RealOption&)option, v->get());
                    } else {
                        reportInvalidType(key.str(), "real", value.type());
                    }
                }
                else if (option.type == OptionType::eSigned) {
                    if (const toml::value<int64_t> *v = value.as_integer()) {
                        detail::updateNumericOption(result, (SignedOption&)option, v->get());
                    } else {
                        reportInvalidType(key.str(), "integer", value.type());
                    }
                }
                else if (option.type == OptionType::eUnsigned) {
                    if (const toml::value<int64_t> *v = value.as_integer()) {
                        detail::updateNumericOption(result, (UnsignedOption&)option, (uint64_t)v->get());
                    } else {
                        reportInvalidType(key.str(), "integer", value.type());
                    }
                }
                else {
                    result.addError(
                        UpdateStatus::eSyntaxError,
                        fmt::format("unexpected value for option {}", key.str())
                    );
                }
            }
        }
    };

    for (const auto& [key, value] : root) {
        if (value.is_table()) {
            acceptTable(mGroupLookup[key], key, *value.as_table());
        } else {
            result.addError(
                UpdateStatus::eSyntaxError,
                fmt::format("only expected toplevel tables {}", key.str())
            );
        }
    }

    return UpdateResult{};
}
