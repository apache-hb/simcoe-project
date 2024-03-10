#include "render/editor/viewport.hpp"

#include "render/render.hpp"

#include "ImGuizmo.h"

#include "core/format.hpp" // IWYU pragma: keep
#include "math/format.hpp" // IWYU pragma: keep

using namespace sm;
using namespace sm::math;
using namespace sm::editor;

static auto gSink = logs::get_sink(logs::Category::eRender);

void ViewportPanel::begin_frame(draw::Camera& camera) {
    ImGuizmo::SetOrthographic(camera.get_projection() == draw::Projection::eOrthographic);
    ImGuizmo::BeginFrame();
}

void ViewportPanel::draw_content() {
    ImGui::Checkbox("Scale to viewport", &mScaleViewport);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (mScaleViewport) {
        float2 sz = { avail.x, avail.y };
        mContext.update_scene_size(sz.as<uint>());
    }

    const auto& handle = mContext.mSceneTargetHandle;
    auto srv = mContext.mFrameGraph.srv(handle);
    auto idx = mContext.mSrvPool.gpu_handle(srv);
    ImGui::Image((ImTextureID)idx.ptr, avail);

    if (mSelected.type != ItemType::eNode) return;

    float2 size = ImGui::GetWindowSize();
    float2 pos = ImGui::GetWindowPos();

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(pos.x, pos.y, size.width, size.height);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(pos, pos + size)) {
        mFlags |= ImGuiWindowFlags_NoMove;
    } else {
        mFlags &= ~ImGuiWindowFlags_NoMove;
    }

    auto& scene = mContext.mWorld.info;
    SM_ASSERTF(mSelected.index < scene.nodes.size(), "Invalid node index: {}", mSelected.index);
    SM_ASSERTF(mSelected.type == ItemType::eNode, "Invalid node type: {}", mSelected.type);
    auto& item = scene.nodes[mSelected.index];

    const auto& [t, r, s] = item.transform;

    auto deg = -math::to_degrees(r);

    // convert our euler order to ImGuizmo's
    float3 euler = { deg.y, deg.z, deg.x };

    float matrix[16];
    ImGuizmo::RecomposeMatrixFromComponents(t.data(), euler.data(), s.data(), matrix);

    auto& camera = mContext.camera;
    float4x4 view = camera.view();
    float4x4 proj = camera.projection(size.width / size.height);

    if (ImGuizmo::Manipulate(view.data(), proj.data(), mOperation, mMode, matrix)) {
        float3 t, r, s;
        ImGuizmo::DecomposeMatrixToComponents(matrix, t.data(), r.data(), s.data());

        // convert back to our euler order
        auto rot = -float3(r.z, r.x, r.y);

        item.transform = { t, degf3(rot), s };
    }

    float2 topright = { pos.x + size.width, pos.y };
    float sz = 128.f;

    ImGuizmo::ViewManipulate(view.data(), 10.f, ImVec2(topright.x - sz, topright.y), ImVec2(sz, sz), 0x10101010);
}

void ViewportPanel::gizmo_settings_panel() {
    if (ImGui::RadioButton("Translate", mOperation == ImGuizmo::TRANSLATE))
		mOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", mOperation == ImGuizmo::ROTATE))
		mOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", mOperation == ImGuizmo::SCALE))
		mOperation = ImGuizmo::SCALE;

    if (!mContext.camera.is_active()) {
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            mOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            mOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_S))
            mOperation = ImGuizmo::SCALE;
    }

    ImGui::BeginDisabled(mOperation == ImGuizmo::SCALE);
    ImGui::Text("Transform Mode");
    ImGui::SameLine();
    if (ImGui::RadioButton("Local", mMode == ImGuizmo::LOCAL))
        mMode = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("World", mMode == ImGuizmo::WORLD))
        mMode = ImGuizmo::WORLD;
    ImGui::EndDisabled();
}

ViewportPanel::ViewportPanel(render::Context &context)
    : IEditorPanel("Viewport")
    , mContext(context)
{ }
