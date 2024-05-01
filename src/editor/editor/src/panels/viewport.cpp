#include "stdafx.hpp"

#include "editor/draw.hpp"

#include "editor/panels/viewport.hpp"

#include "world/ecs.hpp"

#include "ImGuizmo.h"

using namespace sm;
using namespace sm::math;
using namespace sm::math::literals;
using namespace sm::ed;

static constexpr ImGuizmo::OPERATION kRotateXYZ
    = ImGuizmo::ROTATE_X
    | ImGuizmo::ROTATE_Y
    | ImGuizmo::ROTATE_Z;

enum OverlayPosition {
    eOverlayClosed = -1,

    eOverlayTop = (1 << 0),
    eOverlayLeft = (1 << 1),

    eOverlayTopLeft = eOverlayTop | eOverlayLeft,
    eOverlayTopRight = eOverlayTop,
    eOverlayBottomLeft = eOverlayLeft,
    eOverlayBottomRight = 0,
};

#if 0
void ViewportPanel::draw_content() {
    auto& context = *mContext;

    float2 avail = ImGui::GetContentRegionAvail();

    if (mScaleViewport) {
        uint2 sz = avail.as<uint>();

        const world::ecs::Camera *info = mCamera.get<world::ecs::Camera>();
        if (sz != info->window) {
            world::ecs::Camera update = *info;
            update.window = sz;
            mCamera.set(update);
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

    auto deg = math::to_degrees(r.asEulerAngle());

    // convert our euler order to ImGuizmo's
    float3 euler = { deg.y, deg.z, deg.x };

    float matrix[16];
    ImGuizmo::RecomposeMatrixFromComponents(t.data(), euler.data(), s.data(), matrix);

    const world::ecs::Camera *info = mCamera.get<world::ecs::Camera>();

    float4x4 view = world::ecs::getViewMatrix(*mCamera.get<world::ecs::Position, world::ecs::World>(), *mCamera.get<world::ecs::Direction>());
    float4x4 proj = info->getProjectionMatrix();

    if (ImGuizmo::Manipulate(view.data(), proj.data(), mOperation, mMode, matrix)) {
        float3 t, r, s;
        ImGuizmo::DecomposeMatrixToComponents(matrix, t.data(), r.data(), s.data());

        // convert back to our euler order
        auto rot = float3(r.z, r.x, r.y);
        math::quatf q = math::quatf::fromEulerAngle(degf3(rot));

        item.transform = { t, q, s };
    }

    float2 topright = { pos.x + size.width, pos.y };
    float sz = 128.f;

    ImGuizmo::ViewManipulate(view.data(), 10.f, ImVec2(topright.x - sz, topright.y), ImVec2(sz, sz), 0x10101010);
}

void ViewportPanel::gizmo_settings_panel() {
    if (ImGui::RadioButton("Translate", isModeTranslate(mOperation)))
        mOperation = mTranslateOperation;
    ImGui::SameLine();
    draw_gizmo_mode(mTranslateOperation, ImGuizmo::TRANSLATE_X, ImGuizmo::TRANSLATE_Y, ImGuizmo::TRANSLATE_Z);
    ImGui::SameLine();

	if (ImGui::RadioButton("Rotate", isModeRotate(mOperation)))
		mOperation = mRotateOperation;
    ImGui::SameLine();
    drawRotateMode(mOperation);
	ImGui::SameLine();

	if (ImGui::RadioButton("Scale", isModeScale(mOperation)))
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
#endif

// ecs data

struct PrimaryViewport { flecs::entity entity; }; // is this the primary (currently focused) viewport?

static bool isModeTranslate(ImGuizmo::OPERATION op) {
    return op & ImGuizmo::TRANSLATE;
}

static bool isModeRotate(ImGuizmo::OPERATION op) {
    return op & ImGuizmo::ROTATE;
}

static bool isModeScale(ImGuizmo::OPERATION op) {
    return op & ImGuizmo::SCALE;
}

static ImGuizmo::OPERATION setTransformBit(ImGuizmo::OPERATION op, ImGuizmo::OPERATION translate, ImGuizmo::OPERATION rotate, ImGuizmo::OPERATION scale) {
    if (isModeTranslate(op))
        return translate;
    else if (isModeRotate(op))
        return rotate;
    else if (isModeScale(op))
        return scale;
    return op;
}

static ImGuizmo::OPERATION setTransformX(ImGuizmo::OPERATION op) {
    return setTransformBit(op, ImGuizmo::TRANSLATE_X, ImGuizmo::ROTATE_X, ImGuizmo::SCALE_X);
}

static ImGuizmo::OPERATION setTransformY(ImGuizmo::OPERATION op) {
    return setTransformBit(op, ImGuizmo::TRANSLATE_Y, ImGuizmo::ROTATE_Y, ImGuizmo::SCALE_Y);
}

static ImGuizmo::OPERATION setTransfomZ(ImGuizmo::OPERATION op) {
    return setTransformBit(op, ImGuizmo::TRANSLATE_Z, ImGuizmo::ROTATE_Z, ImGuizmo::SCALE_Z);
}

static void drawGizmoMode(ImGuizmo::OPERATION current, ImGuizmo::OPERATION op, ImGuizmo::OPERATION x, ImGuizmo::OPERATION y, ImGuizmo::OPERATION z) {
    if (current & op) {
        ImGui::Text("%s/%s/%s", (op & x) ? "X" : "_", (op & y) ? "Y" : "_", (op & z) ? "Z" : "_");
    } else {
        ImGui::TextDisabled("X/Y/Z");
    }
}

static void drawRotateMode(ImGuizmo::OPERATION operation) {
    bool active = operation & ImGuizmo::ROTATE;
    if (active) {
        ImGui::Text("%s/%s/%s/%s",
            (operation & ImGuizmo::ROTATE_X) ? "X" : "_",
            (operation & ImGuizmo::ROTATE_Y) ? "Y" : "_",
            (operation & ImGuizmo::ROTATE_Z) ? "Z" : "_",
            (operation & ImGuizmo::ROTATE_SCREEN) ? "S" : "_");

    } else {
        ImGui::TextDisabled("X/Y/Z/S");
    }
}

struct ViewportSettings {
    // scaling settings
    bool scaleToViewport = true;

    // overlay panel settings
    OverlayPosition position = eOverlayTopLeft;

    // gizmo settings
    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE mode = ImGuizmo::WORLD;

    ImGuizmo::OPERATION translate = ImGuizmo::TRANSLATE;
    ImGuizmo::OPERATION rotate = kRotateXYZ;
    ImGuizmo::OPERATION scale = ImGuizmo::SCALE;

    void acceptKeyEvents() {
        if (ImGui::IsKeyPressed(ImGuiKey_G))
            operation = translate;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            operation = rotate;
        if (ImGui::IsKeyPressed(ImGuiKey_S))
            operation = scale;

        if (ImGui::IsKeyPressed(ImGuiKey_X))
            operation = setTransformX(operation);
        if (ImGui::IsKeyPressed(ImGuiKey_Y))
            operation = setTransformY(operation);
        if (ImGui::IsKeyPressed(ImGuiKey_Z))
            operation = setTransfomZ(operation);
    }

    void drawLocationPopup() {
        auto drawOption = [&](const char *name, OverlayPosition value) {
            if (ImGui::MenuItem(name, nullptr, position == value))
                position = value;
        };

        if (ImGui::BeginPopupContextWindow()) {
            drawOption("Top Left", eOverlayTopLeft);
            drawOption("Top Right", eOverlayTopRight);
            drawOption("Bottom Left", eOverlayBottomLeft);
            drawOption("Bottom Right", eOverlayBottomRight);
            ImGui::EndPopup();
        }
    }

    void drawSettings() {
        if (ImGui::RadioButton("Translate", isModeTranslate(operation)))
            operation = translate;
        ImGui::SameLine();
        drawGizmoMode(operation, translate, ImGuizmo::TRANSLATE_X, ImGuizmo::TRANSLATE_Y, ImGuizmo::TRANSLATE_Z);
        ImGui::SameLine();

        if (ImGui::RadioButton("Rotate", isModeRotate(operation)))
            operation = rotate;
        ImGui::SameLine();
        drawRotateMode(operation);
        ImGui::SameLine();

        if (ImGui::RadioButton("Scale", isModeScale(operation)))
            operation = scale;
        ImGui::SameLine();
        drawGizmoMode(operation, scale, ImGuizmo::SCALE_X, ImGuizmo::SCALE_Y, ImGuizmo::SCALE_Z);

        ImGui::BeginDisabled(operation == ImGuizmo::SCALE);
        ImGui::Text("Transform Mode");
        ImGui::SameLine();
        if (ImGui::RadioButton("Local", mode == ImGuizmo::LOCAL))
            mode = ImGuizmo::LOCAL;
        ImGui::SameLine();
        if (ImGui::RadioButton("World", mode == ImGuizmo::WORLD))
            mode = ImGuizmo::WORLD;
        ImGui::EndDisabled();
        if (ImGui::CheckboxFlags("Screen space rotation", (int*)&rotate, ImGuizmo::ROTATE_SCREEN)) {
            if (operation & ImGuizmo::ROTATE)
                operation = rotate;
        }
    }
};

struct OverlayCoords {
    float2 position;
    float2 pivot;
};

static OverlayCoords getOverlayPosition(OverlayPosition overlay, float2 size, float2 pos) {
    static constexpr float kWindowSpacing = 10.f;

    float2 pivot = {{
        .x = (overlay & eOverlayLeft) ? 0.f : 1.f,
        .y = (overlay & eOverlayTop) ? 0.f : 1.f,
    }};

    float2 position = {{
        .x = (overlay & eOverlayLeft) ? pos.x + kWindowSpacing : pos.x + size.x - kWindowSpacing,
        .y = (overlay & eOverlayTop) ? pos.y + kWindowSpacing : pos.y + size.y - kWindowSpacing,
    }};

    return { position, pivot };
}

static void drawViewportContent(flecs::entity entity, D3D12_GPU_DESCRIPTOR_HANDLE handle, bool scaleViewport) {
    float2 avail = ImGui::GetContentRegionAvail();

    if (scaleViewport) {
        uint2 sz = avail.as<uint>();

        const world::ecs::Camera *info = entity.get<world::ecs::Camera>();
        if (sz != info->window) {
            world::ecs::Camera update = *info;
            update.window = sz;
            entity.set(update);
        }
    }

    ImGui::Image((ImTextureID)handle.ptr, avail);
}

static bool isActiveViewport(flecs::world& world, flecs::entity entity) {
    return ecs::getPrimaryCamera(world) == entity;
}

void ecs::initWindowComponents(flecs::world &world) {
    // handle to the primary viewport
    world.set<PrimaryViewport>({ });

    world.observer<const world::ecs::Camera>()
        .event(flecs::Monitor)
        .each([](flecs::iter& it, size_t i, const world::ecs::Camera&) {
            flecs::entity entity = it.entity(i);
            if (it.event() == flecs::OnAdd) {
                entity.set<ViewportSettings>({});
            } else if (it.event() == flecs::OnRemove) {
                entity.remove<ViewportSettings>();
            }
        });
}

flecs::entity ecs::addCamera(flecs::world& world, const char *name, math::float3 position, math::float3 direction) {
    return world.entity(name)
        .set<world::ecs::Position, world::ecs::Local>({ position })
        .set<world::ecs::Direction>({ direction })
        .set<world::ecs::Camera>({
            .colour = DXGI_FORMAT_R8G8B8A8_UNORM,
            .depth = DXGI_FORMAT_D32_FLOAT,
            .window = { 800, 600 },
            .fov = 90._deg
        });
}

flecs::entity ecs::getPrimaryCamera(flecs::world& world) {
    const PrimaryViewport *primary = world.get<PrimaryViewport>();
    return primary->entity;
}

size_t ecs::getCameraCount(flecs::world& world) {
    static flecs::query q = world.query_builder().with<world::ecs::Camera>().in().build();
    return q.count();
}

void ecs::drawViewportWindows(render::IDeviceContext& ctx, flecs::world& world) {
    static flecs::query q = world.query<ecs::CameraData, ViewportSettings>();

    static constexpr ImGuiWindowFlags kWindowFlags
        = ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoCollapse;

    static constexpr ImGuiChildFlags kOverlayChildFlags
        = ImGuiChildFlags_Border
        | ImGuiChildFlags_AutoResizeX
        | ImGuiChildFlags_AutoResizeY;

    bool captured = world.get<MouseCaptured>()->captured;

    q.each([&](flecs::entity entity, ecs::CameraData& data, ViewportSettings& gizmo) {
        flecs::string_view name = entity.name();
        render::SrvIndex target = ctx.mFrameGraph.srv(data.target);
        D3D12_GPU_DESCRIPTOR_HANDLE handle = ctx.mSrvPool.gpu_handle(target);

        ImGui::SetNextWindowSizeConstraints(ImVec2(512.f, 512.f), ImVec2(8192.f, 8192.f));

        if (ImGui::Begin(name.c_str(), nullptr, kWindowFlags)) {
            float2 avail = ImGui::GetContentRegionAvail();
            float2 cursor = ImGui::GetCursorScreenPos();

            if (ImGui::IsWindowFocused()) {
                world.set<PrimaryViewport>({ entity });
            }

            if (isActiveViewport(world, entity) && !captured) {
                gizmo.acceptKeyEvents();
            }

            drawViewportContent(entity, handle, true);

            {
                auto [position, pivot] = getOverlayPosition(gizmo.position, avail, cursor);
                ImGui::SetNextWindowPos(position, ImGuiCond_Always, pivot);
                ImGui::SetNextWindowBgAlpha(0.35f);

                if (ImGui::BeginChild("Gizmo Settings", ImVec2(0, 0), kOverlayChildFlags)) {
                    gizmo.drawSettings();
                    gizmo.drawLocationPopup();
                }
                ImGui::EndChild();
            }

            {
                auto [position, pivot] = getOverlayPosition(eOverlayBottomRight, avail, cursor);
                ImGui::SetNextWindowPos(position, ImGuiCond_Always, pivot);
                ImGui::SetNextWindowBgAlpha(0.35f);

                if (ImGui::BeginChild("Camera Info", ImVec2(0, 0), kOverlayChildFlags)) {
                    auto [x, y, z] = entity.get<world::ecs::Position, world::ecs::World>()->position;
                    auto [yaw, pitch, roll] = entity.get<world::ecs::Direction>()->direction;

                    ImGui::Text("Position: %.3f, %.3f, %.3f", x, y, z);
                    ImGui::Text("Direction: %.3f, %.3f, %.3f", yaw, pitch, roll);
                }
                ImGui::EndChild();
            }
        }
        ImGui::End();
    });
}

void ecs::drawViewportMenus(flecs::world& world) {
    static flecs::query cameras = world.query_builder()
        .with<world::ecs::Camera>()
        .build();

    ImGui::SeparatorText("Viewports");

    world.defer([&] {
        cameras.each([&](flecs::entity entity) {
            flecs::string_view name = entity.name();

            bool open = true;
            ImGui::MenuItem(name.c_str(), nullptr, &open);

            if (!open)
                entity.destruct();
        });
    });

    if (ImGui::SmallButton("Add Viewport")) {
        // create the new camera at the same position as the primary camera
        flecs::entity primary = getPrimaryCamera(world);
        const world::ecs::Position *pos = primary.get<world::ecs::Position>();
        const world::ecs::Direction *dir = primary.get<world::ecs::Direction>();

        size_t id = getCameraCount(world);

        addCamera(world, fmt::format("Camera {}", id).c_str(), pos->position, dir->direction);
    }
}
