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

    ImGui_ImplWin32_Init(mConfig.window.get_handle());
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mConfig.swapchain_length),
                        mConfig.swapchain_format, *mSrvHeap,
                        mSrvHeap->GetCPUDescriptorHandleForHeapStart(),
                        mSrvHeap->GetGPUDescriptorHandleForHeapStart());
}

void Context::destroy_imgui() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Context::update_imgui() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    if (ImGui::Begin("Render Config")) {
        int swapchain_length = int_cast<int>(mSwapChainLength);
        if (ImGui::SliderInt("Swapchain Length", &swapchain_length, 2, DXGI_MAX_SWAP_CHAIN_BUFFERS)) {
            update_swapchain_length(int_cast<uint>(swapchain_length));
        }
    }
    ImGui::End();
}

void Context::render_imgui() {
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *mCommandList);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, *mCommandList);
    }
}
