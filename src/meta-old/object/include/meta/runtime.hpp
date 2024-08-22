#pragma once

#include "meta/meta.hpp"
#include "meta/class.hpp"

namespace sm::meta {
    class Runtime {

    public:
        friend Runtime& getRuntime();

        void addClass(const Class& cls);
    };
}
