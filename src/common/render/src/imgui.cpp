#include "stdafx.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::draw;
using namespace sm::render;

void Context::create_imgui() {
    create_imgui_backend();
}

void Context::destroy_imgui() {
    destroy_imgui_backend();
}

void Context::create_imgui_backend() {
    if (!mConfig.imgui) return;

    mImGuiSrvIndex = mSrvPool.allocate();

    ImGui_ImplWin32_Init(mConfig.window.get_handle());

    const auto cpu = mSrvPool.cpu_handle(mImGuiSrvIndex);
    const auto gpu = mSrvPool.gpu_handle(mImGuiSrvIndex);
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mSwapChainLength), mSwapChainFormat,
                        mSrvPool.get(), cpu, gpu);
}

void Context::destroy_imgui_backend() {
    if (!mConfig.imgui) return;

    mSrvPool.release(mImGuiSrvIndex);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}
