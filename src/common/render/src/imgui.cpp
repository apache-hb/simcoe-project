#include "render/render.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx12.h"
#include "imgui/backends/imgui_impl_win32.h"

using namespace sm;
using namespace sm::render;

void Context::create_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    auto& heap = *mSrvHeap.mHeap;

    ImGui_ImplWin32_Init(mConfig.window.get_handle());
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mSwapChainLength),
                        mConfig.swapchain_format, heap,
                        heap->GetCPUDescriptorHandleForHeapStart(),
                        heap->GetGPUDescriptorHandleForHeapStart());
}

void Context::destroy_imgui() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Context::create_imgui_device() {
    ImGui_ImplWin32_Init(mConfig.window.get_handle());

    auto& heap = *mSrvHeap.mHeap;
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mSwapChainLength),
                        mConfig.swapchain_format, heap,
                        heap->GetCPUDescriptorHandleForHeapStart(),
                        heap->GetGPUDescriptorHandleForHeapStart());

    ImGui_ImplDX12_CreateDeviceObjects();
    ImGui_ImplDX12_NewFrame();
}

void Context::destroy_imgui_device() {
    mSink.info("destroying imgui device");
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

void Context::update_imgui() {
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
        const auto& adapters = mInstance.get_adapters();
        for (size_t i = 0; i < adapters.size(); i++) {
            auto& adapter = adapters[i];
            auto name = adapter.name();
            using Reflect = ctu::TypeInfo<AdapterFlag>;

            ImGui::RadioButton(name.data(), &current, int_cast<int>(i));
            ImGui::SameLine();

            if (ImGui::TreeNodeEx((void*)name.data(), ImGuiTreeNodeFlags_DefaultOpen, "")) {
                ImGui::Text("video memory: %s", adapter.vidmem().to_string().c_str());
                ImGui::Text("system memory: %s", adapter.sysmem().to_string().c_str());
                ImGui::Text("shared memory: %s", adapter.sharedmem().to_string().c_str());
                ImGui::Text("flags: %s", Reflect::to_string(adapter.flags()).data());
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();

    if (ImGui::Begin("World Settings")) {
        update_camera();
    }
    ImGui::End();

    ImGui::Render();

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, *mCommandList);
    }

    if (current != (int)mAdapterIndex) {
        update_adapter(current);
    }
}

void Context::render_imgui() {
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *mCommandList);
}
