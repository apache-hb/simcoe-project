#pragma once

#include "render.reflect.h"

namespace sm::render {
    class IRenderObject {
        ObjectDep mDepends;

    protected:
        IRenderObject(ObjectDep depends)
            : mDepends(depends)
        { }

    public:
        virtual ~IRenderObject() = default;

        virtual void create() = 0;
        virtual void destroy() = 0;

        bool depends_on(ObjectDep dep) const {
            return mDepends.any(dep);
        }
    };
}
