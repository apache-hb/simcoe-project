#pragma once

#include "config/option.hpp"

namespace sm::config::detail {
    bool verifyWriteAccess(UpdateResult& errs, const OptionBase& option) {
        if (option.isReadOnly()) {
            errs.errors.push_back(UpdateError{
                UpdateStatus::eReadOnly,
                fmt::format("option {} is read only", option.name)
            });
            return false;
        }

        return true;
    }

    template<typename O, typename T>
        requires (std::derived_from<O, NumericOptionValue<T>>)
    bool updateNumericOption(UpdateResult& errs, O& option, T value) {
        if (!verifyWriteAccess(errs, option))
            return false;

        auto range = option.getCommonRange();
        if (!range.contains(value)) {
            errs.errors.push_back(UpdateError{
                UpdateStatus::eOutOfRange,
                fmt::format("value {} for {} is out of range [{}, {}]", value, option.name, range.min, range.max)
            });
            return false;
        }

        option.setCommonValue(value);

        return true;
    }

    template<std::derived_from<OptionBase> O, typename T>
    bool updateOption(UpdateResult& errs, O& option, T value) {
        if (!verifyWriteAccess(errs, option))
            return false;

        option.setValue(value);

        return true;
    }

    template<typename T>
    const T *getEnumValue(std::span<const EnumValue<T>> options, std::string_view name) {
        fmt::println("options.size: {}", options.size());
        for (const auto& option : options) {
            fmt::println("option.name: {} search: {}", option.name, name);
            if (option.name == name) {
                return &option.value;
            }
        }

        return nullptr;
    }
}
