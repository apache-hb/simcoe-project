#pragma once

#include <stddef.h>

namespace sm::input {
    class Toggle {
        size_t mLastValue = 0;
        bool mActive = false;

    public:
        Toggle(bool initial);
        bool update(size_t value);

        bool is_active() const;
        void activate(bool active);
    };
}
