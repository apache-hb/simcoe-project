#include "stdafx.hpp"

#include "editor/panels/viewport.hpp"

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
    ImGuizmo::SetOrthographic(false);
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

struct OverlayPos {
    float2 position;
    float2 pivot;
};

constexpr float kPad = 10.f;

static OverlayPos get_overlay_position(OverlayPosition overlay, float2 size, float2 pos) {
    float2 pivot = {{
        .x = (overlay & eOverlayLeft) ? 0.f : 1.f,
        .y = (overlay & eOverlayTop) ? 0.f : 1.f,
    }};

    float2 position = {{
        .x = (overlay & eOverlayLeft) ? pos.x + kPad : pos.x + size.x - kPad,
        .y = (overlay & eOverlayTop) ? pos.y + kPad : pos.y + size.y - kPad,
    }};

    return { position, pivot };
}

void ViewportPanel::draw_window() {
    const auto& camera = get_camera();

    if (!camera.is_active()) {
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

    ImGui::SetNextWindowSizeConstraints(ImVec2(200.f, 200.f), ImVec2(8192.f, 8192.f));

    float2 cursor;
    float2 avail;

    if (ImGui::Begin(mName.c_str(), nullptr, mFlags)) {
        cursor = ImGui::GetCursorScreenPos();
        avail = ImGui::GetContentRegionAvail();
        draw_content();
    }
    ImGui::End();

    ImGuiWindowFlags overlay_flags
        = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove;

    auto [position, pivot] = get_overlay_position(mOverlayPosition, avail, cursor);
    ImGui::SetNextWindowPos(position, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowBgAlpha(0.35f);

    if (ImGui::Begin("Gizmo Settings", nullptr, overlay_flags)) {
        gizmo_settings_panel();

        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Top Left", nullptr, mOverlayPosition == eOverlayTopLeft))
                mOverlayPosition = eOverlayTopLeft;
            if (ImGui::MenuItem("Top Right", nullptr, mOverlayPosition == eOverlayTopRight))
                mOverlayPosition = eOverlayTopRight;
            if (ImGui::MenuItem("Bottom Left", nullptr, mOverlayPosition == eOverlayBottomLeft))
                mOverlayPosition = eOverlayBottomLeft;
            if (ImGui::MenuItem("Bottom Right", nullptr, mOverlayPosition == eOverlayBottomRight))
                mOverlayPosition = eOverlayBottomRight;

            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void ViewportPanel::draw_content() {
    auto& camera = get_camera();
    auto& context = *mContext;

    float2 avail = ImGui::GetContentRegionAvail();
    if (mScaleViewport) {
        uint2 sz = avail.as<uint>();
        if (camera.resize(sz)) {
            context.update_framegraph();
        }
    }

    auto srv = context.mFrameGraph.srv(get_target());
    auto idx = context.mSrvPool.gpu_handle(srv);
    ImGui::Image((ImTextureID)idx.ptr, avail);

    if (!context.selected.has_value()) return;
    auto selected = world::get<world::Node>(*context.selected);
    if (selected == world::kInvalidIndex) return;

    float2 size = ImGui::GetWindowSize();
    float2 pos = ImGui::GetWindowPos();

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(pos.x, pos.y, size.width, size.height);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(pos, pos + size)) {
        mFlags |= ImGuiWindowFlags_NoMove;
    } else {
        mFlags &= ~ImGuiWindowFlags_NoMove;
    }

    auto& scene = context.mWorld;
    auto& item = scene.get(selected);

    const auto& [t, r, s] = item.transform;

    auto deg = -math::to_degrees(r.to_euler());

    // convert our euler order to ImGuizmo's
    float3 euler = { deg.y, deg.z, deg.x };

    float matrix[16];
    ImGuizmo::RecomposeMatrixFromComponents(t.data(), euler.data(), s.data(), matrix);

    float4x4 view = camera.view();
    float4x4 proj = camera.projection(size.width / size.height);

    if (ImGuizmo::Manipulate(view.data(), proj.data(), mOperation, mMode, matrix)) {
        float3 t, r, s;
        ImGuizmo::DecomposeMatrixToComponents(matrix, t.data(), r.data(), s.data());

        // convert back to our euler order
        auto rot = -float3(r.z, r.x, r.y);

        item.transform = { t, math::quatf::from_euler(degf3(rot)), s };
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

ViewportPanel::ViewportPanel(ed::EditorContext *context, ed::CameraData *camera)
    : mContext(context)
    , mCamera(camera)
{
    mName = get_camera().name();
}
