#pragma once

#include "config/option.hpp"

namespace sm::config::detail {
    template<typename O, typename T>
        requires (std::derived_from<O, NumericOptionValue<T>>)
    bool updateNumericOption(UpdateResult& errs, O& option, T value) {
        if (option.isReadOnly()) {
            errs.errors.push_back(UpdateError{
                UpdateStatus::eReadOnly,
                std::string{option.name}
            });
            return false;
        }

        if (!option.getCommonRange().contains(value)) {
            errs.errors.push_back(UpdateError{
                UpdateStatus::eOutOfRange,
                std::string{option.name}
            });
            return false;
        }

        option.setCommonValue(value);

        return true;
    }

    template<std::derived_from<OptionBase> O, typename T>
    bool updateOption(UpdateResult& errs, O& option, T value) {
        if (option.isReadOnly()) {
            errs.errors.push_back(UpdateError{
                UpdateStatus::eReadOnly,
                std::string{option.name}
            });
            return false;
        }

        option.setValue(value);

        return true;
    }
}
