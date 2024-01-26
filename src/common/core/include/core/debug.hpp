#pragma once

#include "core/macros.hpp"

namespace sm {
    template<typename T>
    class DebugOnly {
#if SMC_DEBUG
        T m_data;

        void set(const T &data) {
            m_data = data;
        }
#endif

        void set(SM_UNUSED const T &data) {}
    };
}