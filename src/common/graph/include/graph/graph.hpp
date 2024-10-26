#pragma once

#include <directx/d3d12.h>

namespace sm::graph {
    class Handle {

    };

    class RenderPass {
    public:
        Handle create(const char *name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc);
        void read(Handle handle, const char *name, D3D12_RESOURCE_STATES state);
        void write(Handle handle, const char *name, D3D12_RESOURCE_STATES state);
    };

    class RenderGraph {
    public:
        Handle framebuffer();
        RenderPass graphics(const char *name);
    };
}
