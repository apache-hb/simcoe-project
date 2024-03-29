#include "stdafx.hpp"

#include "editor/viewport.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::math;
using namespace sm::ed;

REFLECT_ENUM_BITFLAGS(ImGuizmo::OPERATION, int);

static bool is_mode_translate(ImGuizmo::OPERATION op) {
    return op & ImGuizmo::TRANSLATE;
}

static bool is_mode_rotate(ImGuizmo::OPERATION op) {
    return op & ImGuizmo::ROTATE;
}

static bool is_mode_scale(ImGuizmo::OPERATION op) {
    return op & ImGuizmo::SCALE;
}

static ImGuizmo::OPERATION set_transform_bit(ImGuizmo::OPERATION op, ImGuizmo::OPERATION translate, ImGuizmo::OPERATION rotate, ImGuizmo::OPERATION scale) {
    if (is_mode_translate(op))
        return translate;
    else if (is_mode_rotate(op))
        return rotate;
    else if (is_mode_scale(op))
        return scale;
    return op;
}

static ImGuizmo::OPERATION set_x(ImGuizmo::OPERATION op) {
    return set_transform_bit(op, ImGuizmo::TRANSLATE_X, ImGuizmo::ROTATE_X, ImGuizmo::SCALE_X);
}

static ImGuizmo::OPERATION set_y(ImGuizmo::OPERATION op) {
    return set_transform_bit(op, ImGuizmo::TRANSLATE_Y, ImGuizmo::ROTATE_Y, ImGuizmo::SCALE_Y);
}

static ImGuizmo::OPERATION set_z(ImGuizmo::OPERATION op) {
    return set_transform_bit(op, ImGuizmo::TRANSLATE_Z, ImGuizmo::ROTATE_Z, ImGuizmo::SCALE_Z);
}

void ViewportPanel::begin_frame(draw::Camera& camera) {
    ImGuizmo::SetOrthographic(camera.get_projection() == draw::Projection::eOrthographic);
    ImGuizmo::BeginFrame();
}

void ViewportPanel::draw_gizmo_mode(ImGuizmo::OPERATION op, ImGuizmo::OPERATION x, ImGuizmo::OPERATION y, ImGuizmo::OPERATION z) const {
    bool active = mOperation & op;
    if (active) {
        ImGui::Text("%s/%s/%s", (op & x) ? "X" : "_", (op & y) ? "Y" : "_", (op & z) ? "Z" : "_");
    } else {
        ImGui::TextDisabled("X/Y/Z");
    }
}

void ViewportPanel::draw_rotate_mode() const {
    bool active = mOperation & ImGuizmo::ROTATE;
    if (active) {
        ImGui::Text("%s/%s/%s/%s",
            (mOperation & ImGuizmo::ROTATE_X) ? "X" : "_",
            (mOperation & ImGuizmo::ROTATE_Y) ? "Y" : "_",
            (mOperation & ImGuizmo::ROTATE_Z) ? "Z" : "_",
            (mOperation & ImGuizmo::ROTATE_SCREEN) ? "S" : "_");

    } else {
        ImGui::TextDisabled("X/Y/Z/S");
    }
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
    if (ImGui::RadioButton("Translate", is_mode_translate(mOperation)))
        mOperation = mTranslateOperation;
    ImGui::SameLine();
    draw_gizmo_mode(mTranslateOperation, ImGuizmo::TRANSLATE_X, ImGuizmo::TRANSLATE_Y, ImGuizmo::TRANSLATE_Z);
    ImGui::SameLine();

	if (ImGui::RadioButton("Rotate", is_mode_rotate(mOperation)))
		mOperation = mRotateOperation;
    ImGui::SameLine();
    draw_rotate_mode();
	ImGui::SameLine();

	if (ImGui::RadioButton("Scale", is_mode_scale(mOperation)))
		mOperation = mScaleOperation;
    ImGui::SameLine();
    draw_gizmo_mode(mScaleOperation, ImGuizmo::SCALE_X, ImGuizmo::SCALE_Y, ImGuizmo::SCALE_Z);

    if (!mContext.camera.is_active()) {
        // blender keybinds
        if (ImGui::IsKeyPressed(ImGuiKey_G))
            mOperation = mTranslateOperation;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            mOperation = mRotateOperation;
        if (ImGui::IsKeyPressed(ImGuiKey_S))
            mOperation = mScaleOperation;

        if (ImGui::IsKeyPressed(ImGuiKey_X))
            mOperation = set_x(mOperation);
        if (ImGui::IsKeyPressed(ImGuiKey_Y))
            mOperation = set_y(mOperation);
        if (ImGui::IsKeyPressed(ImGuiKey_Z))
            mOperation = set_z(mOperation);
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
    if (ImGui::CheckboxFlags("Screen space rotation", (int*)&mRotateOperation, ImGuizmo::ROTATE_SCREEN)) {
        if (mOperation & ImGuizmo::ROTATE)
            mOperation = mRotateOperation;
    }
}

ViewportPanel::ViewportPanel(render::Context &context)
    : IEditorPanel("Viewport")
    , mContext(context)
{ }
