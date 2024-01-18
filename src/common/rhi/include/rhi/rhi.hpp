#pragma once

#include <sm_rhi_api.hpp>

#include "rhi.reflect.h"

namespace sm::rhi {
    class SM_RHI_API IContext {
    protected:
        virtual void init(void) = 0;

    public:
        virtual ~IContext() = default;
    };

    SM_RHI_API IContext *get_context(RenderConfig config);
}
