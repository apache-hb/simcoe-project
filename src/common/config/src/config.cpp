#include "stdafx.hpp"
#include "common.hpp"

#include "config/option.hpp"

using namespace sm;
using namespace sm::config;

UpdateResult Context::updateFromConfigFile(std::istream& is) noexcept {
    toml::parse_result parsed = toml::parse(is);
    UpdateResult result;
    if (parsed.failed()) {
        result.errors.emplace_back(UpdateError {
            UpdateStatus::eSyntaxError,
            std::string{parsed.error().description()}
        });
        return result;
    }

    const toml::table& root = parsed.table();

    auto acceptTable = [&](this auto& self, Group *group, std::string_view key, const toml::table& table) {
        if (group == nullptr) {
            result.errors.emplace_back(UpdateError {
                UpdateStatus::eMissingValue,
                fmt::format("no group named {}", key)
            });
            return;
        }

        for (const auto& [key, value] : table) {
            if (value.is_table()) {
                self(mGroupLookup[key], key, *value.as_table());
            }
            else if (value.is_value()) {
                auto it = mArgLookup.find(key);
                if (it == mArgLookup.end()) {
                    result.errors.emplace_back(UpdateError {
                        UpdateStatus::eMissingValue,
                        fmt::format("no option named {}", key.str())
                    });
                    continue;
                }

                auto& option = *it->second;
                if (option.type == OptionType::eBoolean) {
                    if (const toml::value<bool> *v = value.as_boolean()) {
                        detail::updateOption(result, (BoolOption&)option, v->get());
                    } else {
                        result.errors.emplace_back(UpdateError {
                            UpdateStatus::eInvalidValue,
                            fmt::format("expected boolean for option {}", key.str())
                        });
                    }
                }
                else if (option.type == OptionType::eString) {
                    if (const toml::value<std::string> *v = value.as_string()) {
                        detail::updateOption(result, (StringOption&)option, v->get());
                    } else {
                        result.errors.emplace_back(UpdateError {
                            UpdateStatus::eInvalidValue,
                            fmt::format("expected string for option {}", key.str())
                        });
                    }
                } else if (option.type == OptionType::eReal) {
                    if (const toml::value<double> *v = value.as_floating_point()) {
                        detail::updateNumericOption(result, (RealOption&)option, v->get());
                    } else {
                        result.errors.emplace_back(UpdateError {
                            UpdateStatus::eInvalidValue,
                            fmt::format("expected real for option {}", key.str())
                        });
                    }
                } else if (option.type == OptionType::eSigned) {
                    if (const toml::value<int64_t> *v = value.as_integer()) {
                        detail::updateNumericOption(result, (SignedOption&)option, v->get());
                    } else {
                        result.errors.emplace_back(UpdateError {
                            UpdateStatus::eInvalidValue,
                            fmt::format("expected signed integer for option {}", key.str())
                        });
                    }
                } else if (option.type == OptionType::eUnsigned) {
                    if (const toml::value<int64_t> *v = value.as_integer()) {
                        detail::updateNumericOption(result, (UnsignedOption&)option, (uint64_t)v->get());
                    } else {
                        result.errors.emplace_back(UpdateError {
                            UpdateStatus::eInvalidValue,
                            fmt::format("expected unsigned integer for option {}", key.str())
                        });
                    }
                } else {
                    result.errors.emplace_back(UpdateError {
                        UpdateStatus::eSyntaxError,
                        fmt::format("unexpected value for option {}", key.str())
                    });
                }
            }
        }
    };

    for (const auto& [key, value] : root) {
        if (value.is_table()) {
            acceptTable(mGroupLookup[key], key, *value.as_table());
        } else {
            result.errors.emplace_back(UpdateError {
                UpdateStatus::eSyntaxError,
                fmt::format("only expected toplevel tables {}", key.str())
            });
        }
    }

    return UpdateResult{};
}
