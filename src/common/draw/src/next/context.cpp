#include "draw/next/context.hpp"

using namespace sm;
using namespace sm::render::next;
using namespace sm::draw::next;

static render::next::ContextConfig updateConfig(render::next::ContextConfig current) {
    render::next::ContextConfig config = current;
    config.srvHeapSize += 1;
    return config;
}

DrawContext::DrawContext(render::next::ContextConfig config, HWND hwnd)
    : Super(updateConfig(config))
    , mImGui(addResource<ImGuiDrawContext>(hwnd))
{
    mImGui->setup();
    mImGui->create();
}

DrawContext::~DrawContext() noexcept try {
    flushDeviceForCleanup();
    mImGui->destroy();
} catch (...) {
    LOG_ERROR(RenderLog, "Failed to destroy DrawContext");
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
