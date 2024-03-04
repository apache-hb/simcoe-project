#include "render/editor/viewport.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

static auto gSink = logs::get_sink(logs::Category::eRender);

void ViewportPanel::draw_content() {
    const auto& handle = mContext.mSceneTargetHandle;
    auto srv = mContext.mFrameGraph.srv(handle);
    auto idx = mContext.mSrvPool.gpu_handle(srv);

    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::Image((ImTextureID)idx.ptr, size);
}

ViewportPanel::ViewportPanel(render::Context &context)
    : IEditorPanel("Viewport")
    , mContext(context)
{ }
