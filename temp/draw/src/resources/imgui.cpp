#include "draw/resources/imgui.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

using namespace sm;
using namespace sm::render::next;
using namespace sm::draw::next;


bool ImGuiDrawContext::hasViewportSupport() const noexcept {
    ImGuiIO& io = ImGui::GetIO();
    return io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable;
}

void ImGuiDrawContext::setup() noexcept {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (hasViewportSupport()) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void ImGuiDrawContext::setupPlatform() noexcept {
    ImGui_ImplWin32_Init(mWindow);
}

void ImGuiDrawContext::setupRender(ID3D12Device *device, DXGI_FORMAT format, UINT frames, render::next::DescriptorPool& srvHeap) noexcept {
    mSrvHeapIndex = srvHeap.allocate();

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap.device(mSrvHeapIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap.host(mSrvHeapIndex);
    ImGui_ImplDX12_Init(device, frames, format, srvHeap.get(), cpuHandle, gpuHandle);
}

void ImGuiDrawContext::destroy() noexcept {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiDrawContext::begin() noexcept {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiDrawContext::end(ID3D12GraphicsCommandList *list) noexcept {
    ImGui::Render();

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list);
}

void ImGuiDrawContext::updatePlatformViewports(ID3D12GraphicsCommandList *list) noexcept {
    if (hasViewportSupport()) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)list);
    }
}

void ImGuiDrawContext::reset() noexcept {
    DescriptorPool &srvHeap = mContext.getSrvHeap();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();

    srvHeap.free(mSrvHeapIndex);
}

void ImGuiDrawContext::create() {
    SurfaceInfo info = mContext.getSwapChainInfo();
    DescriptorPool &srvHeap = mContext.getSrvHeap();

    setupPlatform();
    setupRender(mContext.getDevice(), info.format, info.length, srvHeap);
}

void ImGuiDrawContext::update(SurfaceInfo info) {
    // TODO: only update when info.length has changed
    DescriptorPool &srvHeap = mContext.getSrvHeap();

    reset();
    setupPlatform();
    setupRender(mContext.getDevice(), info.format, info.length, srvHeap);
}
