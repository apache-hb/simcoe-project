#include "test/render_test_common.hpp"
#include "render/next/viewport.hpp"

#include "graph/graph.hpp"
#include "math/colour.hpp"

#include <directx/d3dx12_core.h>

using namespace sm::math;
using namespace sm::graph;
using namespace sm::render::next;

class ImGuiPass final : public graph::RenderPass {
    Attachment mOutput;

public:
    ImGuiPass(const RenderPassInfo& info)
        : RenderPass(info, D3D12_COMMAND_LIST_TYPE_DIRECT)
    { }

    Attachment getOutput() const { return mOutput; }
};

class BlitPass final : public graph::RenderPass {
    Attachment mInput;
    Resource mOutput;

public:
    BlitPass(const RenderPassInfo& info)
        : RenderPass(info, D3D12_COMMAND_LIST_TYPE_DIRECT)
    { }

    Attachment getInput() const { return mInput; }
    Resource getOutput() const { return mOutput; }
};

class OpaquePass final : public graph::RenderPass {
    Resource mOutput;

public:
    OpaquePass(const RenderPassInfo& info)
        : RenderPass(info, D3D12_COMMAND_LIST_TYPE_DIRECT)
    { }

    Resource getOutput() const { return mOutput; }
};
