#include "render/render.hpp"

#include "imgui/backends/imgui_impl_dx12.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/imgui.h"
#include "implot.h"

using namespace sm;
using namespace sm::draw;
using namespace sm::render;

namespace MyGui {
template<ctu::Reflected T>
    requires (ctu::is_enum<T>())
static bool CheckboxFlags(const char *label, T &flags, T flag) {
    unsigned val = flags.as_integral();
    if (ImGui::CheckboxFlags(label, &val, flag.as_integral())) {
        flags = T(val);
        return true;
    }
    return false;
}

template <ctu::Reflected T>
    requires(ctu::is_enum<T>())
static bool EnumCombo(const char *label, typename ctu::TypeInfo<T>::Type &choice) {
    using Reflect = ctu::TypeInfo<T>;
    const auto id = Reflect::to_string(choice);
    if (ImGui::BeginCombo(label, id.c_str())) {
        for (size_t i = 0; i < std::size(Reflect::kCases); i++) {
            const auto &[name, value] = Reflect::kCases[i];
            bool selected = choice == value;
            if (ImGui::Selectable(name.c_str(), selected)) {
                choice = value;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return true;
}

SM_UNUSED
static bool SliderAngle3(const char *label, Radians<float3> &value, radf min, radf max) {
    auto degrees = math::to_degrees(value);
    if (ImGui::SliderFloat3(label, degrees.data(), min.get_degrees(), max.get_degrees())) {
        value = Radians(math::to_radians(degrees));
        return true;
    }
    return false;
}

SM_UNUSED
static bool SliderQuat(SM_UNUSED const char *label, SM_UNUSED quatf &value, SM_UNUSED radf min, SM_UNUSED radf max) {
#if 0
    float3 euler = math::to_euler(value);
    if (ImGui::SliderFloat3(label, euler.data(), min, max)) {
        value = math::from_euler(euler);
        return true;
    }
    return false;
#endif
    return false;
}
} // namespace MyGui

static constexpr MeshInfo get_default_info(MeshType type) {
    switch (type.as_enum()) {
    case MeshType::eCube: return {.type = type, .cube = {1.f, 1.f, 1.f}};
    case MeshType::eSphere: return {.type = type, .sphere = {1.f, 6, 6}};
    case MeshType::eCylinder: return {.type = type, .cylinder = {1.f, 1.f, 8}};
    case MeshType::ePlane: return {.type = type, .plane = {1.f, 1.f}};
    case MeshType::eWedge: return {.type = type, .wedge = {1.f, 1.f, 1.f}};
    case MeshType::eCapsule: return {.type = type, .capsule = {1.f, 5.f}};
    case MeshType::eGeoSphere: return {.type = type, .geosphere = {1.f, 2}};
    default: return {.type = type};
    }
}

void Context::create_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImPlot::CreateContext();
    ImPlot::StyleColorsDark();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    auto cases = MeshType::cases();
    for (MeshType i : cases) {
        mMeshCreateInfo[i] = get_default_info(i);
    }

    create_imgui_backend();
}

void Context::destroy_imgui() {
    destroy_imgui_backend();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void Context::create_imgui_backend() {
    mImGuiSrvIndex = mSrvPool.allocate();

    ImGui_ImplWin32_Init(mConfig.window.get_handle());

    const auto cpu = mSrvPool.cpu_handle(mImGuiSrvIndex);
    const auto gpu = mSrvPool.gpu_handle(mImGuiSrvIndex);
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mSwapChainLength), mSwapChainFormat,
                        mSrvPool.get(), cpu, gpu);
}

void Context::destroy_imgui_backend() {
    mSrvPool.release(mImGuiSrvIndex);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

bool Context::update_imgui() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    mEditor.draw();

    if (ImGui::Begin("World Settings")) {
        ImGui::SeparatorText("Camera");
        camera.draw_debug();

        ImGui::SeparatorText("Scene");

        if (ImGui::Button("Create new primitive")) {
            ImGui::OpenPopup("New Primitive");
        }

        if (ImGui::BeginPopup("New Primitive")) {
            static draw::MeshType type = draw::MeshType::eCube;
            static float3 colour = {1.f, 1.f, 1.f};
            MyGui::EnumCombo<draw::MeshType>("Type", type);

            auto &info = mMeshCreateInfo[type];

            switch (type.as_enum()) {
            case draw::MeshType::eCube:
                ImGui::SliderFloat("Width", &info.cube.width, 0.1f, 10.f);
                ImGui::SliderFloat("Height", &info.cube.height, 0.1f, 10.f);
                ImGui::SliderFloat("Depth", &info.cube.depth, 0.1f, 10.f);
                break;
            case draw::MeshType::eSphere:
                ImGui::SliderFloat("Radius", &info.sphere.radius, 0.1f, 10.f);
                ImGui::SliderInt("Slices", &info.sphere.slices, 3, 32);
                ImGui::SliderInt("Stacks", &info.sphere.stacks, 3, 32);
                break;
            case draw::MeshType::eCylinder:
                ImGui::SliderFloat("Radius", &info.cylinder.radius, 0.1f, 10.f);
                ImGui::SliderFloat("Height", &info.cylinder.height, 0.1f, 10.f);
                ImGui::SliderInt("Slices", &info.cylinder.slices, 3, 32);
                break;
            case draw::MeshType::ePlane:
                ImGui::SliderFloat("Width", &info.plane.width, 0.1f, 10.f);
                ImGui::SliderFloat("Depth", &info.plane.depth, 0.1f, 10.f);
                break;
            case draw::MeshType::eWedge:
                ImGui::SliderFloat("Width", &info.wedge.width, 0.1f, 10.f);
                ImGui::SliderFloat("Depth", &info.wedge.depth, 0.1f, 10.f);
                ImGui::SliderFloat("Height", &info.wedge.height, 0.1f, 10.f);
                break;
            case draw::MeshType::eCapsule:
                ImGui::SliderFloat("Radius", &info.capsule.radius, 0.1f, 10.f);
                ImGui::SliderFloat("Height", &info.capsule.height, 0.1f, 10.f);
                break;
            case draw::MeshType::eGeoSphere:
                ImGui::SliderFloat("Radius", &info.geosphere.radius, 0.1f, 10.f);
                ImGui::SliderInt("Subdivisions", &info.geosphere.subdivisions, 1, 8);
                break;
            default: ImGui::Text("Unimplemented primitive type"); break;
            }

            ImGui::ColorEdit3("Colour", colour.data(), ImGuiColorEditFlags_Float);

            if (ImGui::Button("Create")) {
                mMeshes.push_back(create_mesh(info, colour));
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        for (auto &primitive : mMeshes) {
            const auto &info = primitive.mInfo;
            using Reflect = ctu::TypeInfo<draw::MeshType>;
            auto name = Reflect::to_string(info.type);
            if (ImGui::TreeNodeEx((void *)&primitive, ImGuiTreeNodeFlags_DefaultOpen, "%s",
                                  name.data())) {
                auto &[position, rotation, scale] = primitive.mTransform;
                ImGui::SliderFloat3("Position", position.data(), -10.f, 10.f);
                MyGui::SliderAngle3("Rotation", rotation, -180._deg, 180._deg);
                ImGui::SliderFloat3("Scale", scale.data(), 0.1f, 10.f);

                switch (info.type.as_enum()) {
                case MeshType::eCube:
                    ImGui::Text("Width: %f", info.cube.width);
                    ImGui::Text("Height: %f", info.cube.height);
                    ImGui::Text("Depth: %f", info.cube.depth);
                    break;
                case MeshType::eSphere:
                    ImGui::Text("Radius: %f", info.sphere.radius);
                    ImGui::Text("Slices: %d", info.sphere.slices);
                    ImGui::Text("Stacks: %d", info.sphere.stacks);
                    break;
                case MeshType::eCylinder:
                    ImGui::Text("Radius: %f", info.cylinder.radius);
                    ImGui::Text("Height: %f", info.cylinder.height);
                    ImGui::Text("Slices: %d", info.cylinder.slices);
                    break;
                case MeshType::ePlane:
                    ImGui::Text("Width: %f", info.plane.width);
                    ImGui::Text("Depth: %f", info.plane.depth);
                    break;
                case MeshType::eWedge:
                    ImGui::Text("Width: %f", info.wedge.width);
                    ImGui::Text("Height: %f", info.wedge.height);
                    ImGui::Text("Depth: %f", info.wedge.depth);
                    break;
                case MeshType::eCapsule:
                    ImGui::Text("Radius: %f", info.capsule.radius);
                    ImGui::Text("Height: %f", info.capsule.height);
                    break;
                case MeshType::eGeoSphere:
                    ImGui::Text("Radius: %f", info.geosphere.radius);
                    ImGui::Text("Subdivisions: %d", info.geosphere.subdivisions);
                    break;
                case MeshType::eImported: ImGui::Text("Imported"); break;
                default: ImGui::Text("Unknown primitive type"); break;
                }
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();

    ImGui::Render();

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, *mCommandList);
    }

    if (mDeviceLost) {
        recreate_device();
        return false;
    }

    return true;
}

void Context::render_imgui() {
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *mCommandList);
}
