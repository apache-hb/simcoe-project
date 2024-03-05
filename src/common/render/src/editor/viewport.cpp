#include "render/editor/viewport.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::math;
using namespace sm::editor;

static auto gSink = logs::get_sink(logs::Category::eRender);

void ViewportPanel::draw_content() {
    ImGui::Checkbox("Scale to viewport", &mScaleViewport);

    ImVec2 size = ImGui::GetContentRegionAvail();
    if (mScaleViewport) {
        float2 sz = { size.x, size.y };
        mContext.update_scene_size(sz.as<uint>());
    }

    const auto& handle = mContext.mSceneTargetHandle;
    auto srv = mContext.mFrameGraph.srv(handle);
    auto idx = mContext.mSrvPool.gpu_handle(srv);
    ImGui::Image((ImTextureID)idx.ptr, size);
}

ViewportPanel::ViewportPanel(render::Context &context)
    : IEditorPanel("Viewport")
    , mContext(context)
{ }
