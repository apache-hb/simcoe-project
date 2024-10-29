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

DrawContext::DrawContext(render::next::ContextConfig config, HWND hwnd, math::uint2 resolution)
    : Super(updateConfig(config))
    , mImGui(addResource<ImGuiDrawContext>(hwnd))
    , mCompute(addResource<ComputeContext>())
    , mVic20(addResource<Vic20Display>(resolution))
{
    mImGui->setup();
    mImGui->create();
    mCompute->create();
    mVic20->create();
}

void DrawContext::begin() {
    beginCommands();

    mImGui->begin();
}

void DrawContext::end() {
    mCompute->begin(mCurrentBackBuffer);
    mVic20->record(mCompute->getCommandList(), mCurrentBackBuffer);
    mCompute->end();

    // block the direct queue until all compute work is done
    mCompute->blockQueueUntil(mDirectQueue.get());

    ID3D12Resource *vic20 = mVic20->getTarget();

    const D3D12_RESOURCE_BARRIER cIntoTexture[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            vic20,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        )
    };

    const D3D12_RESOURCE_BARRIER cIntoUnordered[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            vic20,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        )
    };

    ID3D12GraphicsCommandList *list = mDirectCommandSet->get();

    list->ResourceBarrier(_countof(cIntoTexture), cIntoTexture);
    mImGui->end(list);
    list->ResourceBarrier(_countof(cIntoUnordered), cIntoUnordered);

    endCommands();
    executeCommands();

    mCompute->waitOnQueue(mDirectQueue.get());

    mImGui->updatePlatformViewports(list);

    presentSurface();
}
