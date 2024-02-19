#include "render/render.hpp"

#include "imgui/backends/imgui_impl_dx12.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/imgui.h"


using namespace sm;
using namespace sm::draw;
using namespace sm::render;

namespace MyGui {
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

static bool SliderAngle3(const char *label, float3 &value, float min, float max) {
    float3 degrees = to_degrees(value);
    if (ImGui::SliderFloat3(label, &degrees.x, min, max)) {
        value = to_radians(degrees);
        return true;
    }
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

    ImGui_ImplWin32_Init(mConfig.window.get_handle());

    const auto cpu = mSrvHeap.cpu_descriptor_handle(eDescriptorImGui);
    const auto gpu = mSrvHeap.gpu_descriptor_handle(eDescriptorImGui);
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mSwapChainLength), mConfig.swapchain_format,
                        *mSrvHeap, cpu, gpu);

    auto cases = MeshType::cases();
    for (MeshType i : cases) {
        mMeshCreateInfo[i] = get_default_info(i);
    }
}

void Context::destroy_imgui() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Context::create_imgui_backend() {
    ImGui_ImplWin32_Init(mConfig.window.get_handle());

    const auto cpu = mSrvHeap.cpu_descriptor_handle(eDescriptorImGui);
    const auto gpu = mSrvHeap.gpu_descriptor_handle(eDescriptorImGui);
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mSwapChainLength), mConfig.swapchain_format,
                        *mSrvHeap, cpu, gpu);

    ImGui_ImplDX12_CreateDeviceObjects();
    ImGui_ImplDX12_NewFrame();
}

void Context::destroy_imgui_backend() {
    ImGui_ImplDX12_InvalidateDeviceObjects();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void Context::update_camera() {
    ImGui::SeparatorText("Camera");

    ImGui::SliderFloat("fov", &mCamera.fov, 0.1f, 3.14f);
    ImGui::SliderFloat3("position", &mCamera.position.x, -10.f, 10.f);
    ImGui::SliderFloat3("direction", &mCamera.direction.x, -1.f, 1.f);
    ImGui::SliderFloat("speed", &mCamera.speed, 0.1f, 10.f);
}

static void display_mem_budget(const D3D12MA::Budget &budget) {
    sm::Memory usage_bytes = budget.UsageBytes;
    sm::Memory budget_bytes = budget.BudgetBytes;
    ImGui::Text("Usage: %s", usage_bytes.to_string().c_str());
    ImGui::Text("Budget: %s", budget_bytes.to_string().c_str());

    uint64 alloc_count = budget.Stats.AllocationCount;
    sm::Memory alloc_bytes = budget.Stats.AllocationBytes;
    uint64 block_count = budget.Stats.BlockCount;
    sm::Memory block_bytes = budget.Stats.BlockBytes;

    ImGui::Text("Allocated blocks: %llu", alloc_count);
    ImGui::Text("Allocated: %s", alloc_bytes.to_string().c_str());

    ImGui::Text("Block count: %llu", block_count);
    ImGui::Text("Block: %s", block_bytes.to_string().c_str());
}

bool Context::update_imgui() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    int current = int_cast<int>(mAdapterIndex);
    if (ImGui::Begin("Render Config")) {
        int backbuffers = int_cast<int>(mSwapChainLength);
        if (ImGui::SliderInt("Swapchain Length", &backbuffers, 2, DXGI_MAX_SWAP_CHAIN_BUFFERS)) {
            update_swapchain_length(int_cast<uint>(backbuffers));
        }

        ImGui::SeparatorText("Adapters");
        const auto &adapters = mInstance.get_adapters();
        for (size_t i = 0; i < adapters.size(); i++) {
            auto &adapter = adapters[i];
            auto name = adapter.name();
            using Reflect = ctu::TypeInfo<AdapterFlag>;

            char label[32];
            (void)snprintf(label, sizeof(label), "##%zu", i);

            ImGui::RadioButton(label, &current, int_cast<int>(i));
            ImGui::SameLine();

            if (ImGui::TreeNodeEx((void *)name.data(), ImGuiTreeNodeFlags_None, "%s",
                                  name.data())) {
                ImGui::Text("Video memory: %s", adapter.vidmem().to_string().c_str());
                ImGui::Text("System memory: %s", adapter.sysmem().to_string().c_str());
                ImGui::Text("Shared memory: %s", adapter.sharedmem().to_string().c_str());
                ImGui::Text("Flags: %s", Reflect::to_string(adapter.flags()).data());
                ImGui::TreePop();
            }
        }

        ImGui::SeparatorText("Render Status");
        if (ImGui::CollapsingHeader("Allocator")) {
            ImGui::SeparatorText("Memory");

            {
                sm::Memory local = mAllocator->GetMemoryCapacity(DXGI_MEMORY_SEGMENT_GROUP_LOCAL);
                sm::Memory nonlocal = mAllocator->GetMemoryCapacity(
                    DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL);

                ImGui::Text("Local: %s", local.to_string().c_str());
                ImGui::Text("Non-Local: %s", nonlocal.to_string().c_str());
            }

            {
                const char *mode = mAllocator->IsCacheCoherentUMA() ? "Cache Coherent UMA"
                                   : mAllocator->IsUMA()            ? "Unified Memory Architecture"
                                                                    : "Non-UMA";

                ImGui::Text("UMA: %s", mode);
            }

            ImGui::SeparatorText("Budget");
            {
                D3D12MA::Budget local;
                D3D12MA::Budget nolocal;
                mAllocator->GetBudget(&local, &nolocal);

                ImVec2 avail = ImGui::GetContentRegionAvail();
                ImGuiStyle &style = ImGui::GetStyle();

                float width = avail.x / 2.f - style.ItemSpacing.x;

                ImGui::BeginChild("Local Budget", ImVec2(width, 150), ImGuiChildFlags_Border);
                ImGui::SeparatorText("Local");
                display_mem_budget(local);
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("Non-Local Budget", ImVec2(width, 150), ImGuiChildFlags_Border);
                ImGui::SeparatorText("Non-Local");
                display_mem_budget(nolocal);
                ImGui::EndChild();
            }
        }

        if (ImGui::CollapsingHeader("Descriptor heaps")) {
            ImGui::Text("RTV capacity: %u", mRtvHeap.mCapacity);
            ImGui::Text("DSV capacity: %u", mDsvHeap.mCapacity);
            ImGui::Text("SRV capacity: %u", mSrvHeap.mCapacity);
        }
    }
    ImGui::End();

    if (ImGui::Begin("World Settings")) {
        update_camera();

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
                mPrimitives.push_back(create_mesh(info, colour));
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        for (auto &primitive : mPrimitives) {
            const auto &info = primitive.mInfo;
            using Reflect = ctu::TypeInfo<draw::MeshType>;
            auto name = Reflect::to_string(info.type);
            if (ImGui::TreeNodeEx((void *)&primitive, ImGuiTreeNodeFlags_DefaultOpen, "%s",
                                  name.data())) {
                auto &[position, rotation, scale] = primitive.mTransform;
                ImGui::SliderFloat3("Position", position.data(), -10.f, 10.f);
                MyGui::SliderAngle3("Rotation", rotation, -3.14f, 3.14f);
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

    if (current != (int)mAdapterIndex) {
        update_adapter(current);
        return false;
    }

    return true;
}

void Context::render_imgui() {
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *mCommandList);
}
