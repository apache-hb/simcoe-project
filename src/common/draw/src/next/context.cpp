#include "draw/next/context.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

using namespace sm;
using namespace sm::render::next;
using namespace sm::draw::next;

static render::next::ContextConfig updateConfig(render::next::ContextConfig current) {
    render::next::ContextConfig config = current;
    config.srvHeapSize = 1;
    return config;
}

DrawContext::DrawContext(render::next::ContextConfig config, HWND hwnd)
    : Super(updateConfig(config))
    , mImGui(addResource<ImGuiDrawContext>(hwnd))
{
    mImGui->setup();
    mImGui->create();
}

DrawContext::~DrawContext() noexcept {
    mImGui->destroy();
}

void ImGuiDrawContext::setup() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void ImGuiDrawContext::setupPlatform() {
    ImGui_ImplWin32_Init(mWindow);
}

void ImGuiDrawContext::setupRender(ID3D12Device *device, DXGI_FORMAT format, UINT frames, render::next::DescriptorPool& srvHeap, size_t srvHeapIndex) {
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap.getDeviceDescriptorHandle(srvHeapIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap.getHostDescriptorHandle(srvHeapIndex);
    ImGui_ImplDX12_Init(device, frames, format, srvHeap.get(), cpuHandle, gpuHandle);
}

void ImGuiDrawContext::destroy() noexcept {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiDrawContext::begin() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiDrawContext::end(ID3D12GraphicsCommandList *list) {
    ImGui::Render();

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), list);

    ImGuiIO& io = ImGui::GetIO();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)list);
    }
}

void ImGuiDrawContext::reset() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void ImGuiDrawContext::create() {
    SurfaceInfo info = mContext.getSwapChainInfo();
    DescriptorPool &srvHeap = mContext.getSrvHeap();

    setupPlatform();
    setupRender(mContext.getDevice(), info.format, info.length, srvHeap, 0);
}

void ImGuiDrawContext::update(SurfaceInfo info) {
    // TODO: only update when info.length has changed
    DescriptorPool &srvHeap = mContext.getSrvHeap();
    ImGui_ImplDX12_InvalidateDeviceObjects();
    ImGui_ImplDX12_CreateDeviceObjects();
    setupRender(mContext.getDevice(), info.format, info.length, srvHeap, 0);
}

void DrawContext::begin() {
    Super::begin();
    mImGui->begin();
}

void DrawContext::end() {
    mImGui->end(mDirectCommandSet->get());
    Super::end();
}
