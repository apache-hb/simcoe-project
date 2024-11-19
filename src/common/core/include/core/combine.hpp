#pragma once

#include "core/core.hpp"
#include "core/macros.hpp"

namespace sm {
    template<typename T, typename... U> requires ((std::derived_from<U, T> && ...) && std::has_virtual_destructor_v<T>)
    class Combine {
        union CompositeStorage {
            alignas(std::max(alignof(U)...)) char derived[std::max(sizeof(U)...)];

            ~CompositeStorage() noexcept {}
        };

        CompositeStorage storage;

        T *pointer() noexcept { return reinterpret_cast<T *>(storage.derived); }
        T **address() noexcept { return reinterpret_cast<T **>(&storage.derived); }

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
