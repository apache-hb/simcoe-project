#pragma once

#include <variant>

namespace sm {
    template<typename... T>
    using Variant = std::variant<T...>;
}
