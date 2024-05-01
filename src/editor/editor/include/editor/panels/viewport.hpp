#pragma once

#include <flecs.h>

namespace sm::render {
    struct IDeviceContext;
}

namespace sm::ed {
    namespace ecs {
        // init
        void initWindowComponents(flecs::world& world);

        // update camera related systems/components
        flecs::entity addCamera(flecs::world& world, const char *name, math::float3 position, math::float3 direction);

        // query cameras and related components
        flecs::entity getPrimaryCamera(flecs::world& world);
        size_t getCameraCount(flecs::world& world);

        // editor draw functions
        void drawViewportWindows(render::IDeviceContext& ctx, flecs::world& world);
        void drawViewportMenus(flecs::world& world);
    }
}
