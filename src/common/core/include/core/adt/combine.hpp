#pragma once

#include "core/core.hpp"
#include "core/macros.hpp"

namespace sm::adt {
    template<typename T, typename... U> requires ((std::derived_from<U, T> && ...) && std::has_virtual_destructor_v<T>)
    class Combine {
        alignas(std::max(alignof(U)...)) char storage[std::max(sizeof(U)...)];

        T *pointer() noexcept { return reinterpret_cast<T *>(storage); }
        T **address() noexcept { return reinterpret_cast<T **>(&storage); }

    public:
        SM_NOCOPY(Combine);
        SM_NOMOVE(Combine);

        template<typename O> requires (std::same_as<O, U> || ...)
        Combine(O &&o) {
            new (address()) O{std::forward<O>(o)};
        }

        ~Combine() noexcept {
            pointer()->~T();
        }

        T *operator->() noexcept { return pointer(); }
        T **operator&() noexcept { return address(); }
    };
}
