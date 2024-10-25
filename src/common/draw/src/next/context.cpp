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
    , mCompute(addResource<ComputeContext>())
{
    mImGui->setup();
    mImGui->create();
    mCompute->setup(getSwapChainLength());
}

DrawContext::~DrawContext() noexcept {
    mImGui->destroy();
}

void DrawContext::begin() {
    Super::begin();
    mImGui->begin();
}

void DrawContext::end() {
    mImGui->end(mDirectCommandSet->get());
    Super::end();
}
