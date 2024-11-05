#pragma once

#include <exception>
#include <expected>

namespace sm::errors {
    template<typename E = std::exception, typename F> requires (std::is_base_of_v<std::exception, E>)
    std::expected<std::invoke_result_t<F>, E> maybe(F&& func) {
        try {
            return std::invoke(std::forward<F>(func));
        } catch (E& e) {
            return std::unexpected(e);
        }
    }
}
