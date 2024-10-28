#include "draw/next/context.hpp"

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
    , mVic20(addResource<Vic20Display>())
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

    ID3D12GraphicsCommandList *list = mDirectCommandSet->get();
    mImGui->end(list);

    endCommands();
    executeCommands();

    mImGui->updatePlatformViewports(list);

    presentSurface();
}
