#pragma once

#include "render/next/graph/pass.hpp"
#include "render/next/graph/handle.hpp"

namespace sm::render::next::graph {
    class RenderGraph {
    public:
        Handle newHandle(D3D12_RESOURCE_STATES state, std::string name);
        RenderPassBuilder newPass(D3D12_COMMAND_LIST_TYPE type, std::string name);
    };
}
