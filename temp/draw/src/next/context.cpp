#include "draw/next/context.hpp"

#include <directx/d3dx12_barriers.h>

using namespace sm;
using namespace sm::render::next;
using namespace sm::draw::next;

static render::next::ContextConfig updateConfig(render::next::ContextConfig current) {
    render::next::ContextConfig config = current;
    config.srvHeapSize += 100;
    return config;
}

DrawContext::DrawContext(render::next::ContextConfig config, HWND hwnd)
    : Super(updateConfig(config))
    , mImGui(addResource<ImGuiDrawContext>(hwnd))
    , mCompute(addResource<ComputeContext>())
{
    mImGui->setup();
    mImGui->create();
    mCompute->create();
}

void DrawContext::begin() {
    beginCommands();

    mImGui->begin();
}

void DrawContext::end() {
    ID3D12GraphicsCommandList *list = mDirectCommandSet->get();

    mImGui->end(list);

    endCommands();
    executeCommands();

    mImGui->updatePlatformViewports(list);

    presentSurface();
}
