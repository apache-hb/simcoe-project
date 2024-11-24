#pragma once

namespace sm::adt {
    template<typename T>
    union MaybeStorage {
        ~MaybeStorage() noexcept { }
        T value;
    };

    template<typename T>
    class Maybe {

    };
}
