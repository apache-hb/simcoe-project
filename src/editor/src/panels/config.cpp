#include "stdafx.hpp"

#include "editor/config.hpp"

#include "editor/draw.hpp"

using namespace sm;
using namespace sm::ed;

static constexpr int kMaxLength = DXGI_MAX_SWAP_CHAIN_BUFFERS;

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
} // namespace MyGui

void RenderConfig::draw_adapters() const {
    const auto& adapters = mContext.mInstance.get_adapters();
    for (size_t i = 0; i < adapters.size(); i++) {
        auto &adapter = adapters[i];
        auto name = adapter.name();
        using Reflect = ctu::TypeInfo<render::AdapterFlag>;

        char label[32];
        (void)snprintf(label, sizeof(label), "##%zu", i);

        if (ImGui::RadioButton(label, (int*)&mContext.mAdapterIndex, int_cast<int>(i))) {
            mContext.set_device_lost();
        }
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

void RenderConfig::draw_allocator_info() const {
    auto& allocator = mContext.mAllocator;

    {
        sm::Memory local = allocator->GetMemoryCapacity(DXGI_MEMORY_SEGMENT_GROUP_LOCAL);
        sm::Memory nonlocal = allocator->GetMemoryCapacity(
            DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL);

        ImGui::Text("Local: %s", local.to_string().c_str());
        ImGui::Text("Non-Local: %s", nonlocal.to_string().c_str());
    }

    {
        const char *mode = allocator->IsCacheCoherentUMA() ? "Cache Coherent UMA"
                            : allocator->IsUMA()            ? "Unified Memory Architecture"
                                                            : "Non-UMA";

        ImGui::Text("UMA: %s", mode);
    }

    ImGui::SeparatorText("Budget");
    {
        D3D12MA::Budget local;
        D3D12MA::Budget nolocal;
        allocator->GetBudget(&local, &nolocal);

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

bool RenderConfig::draw_debug_flags() const {
    using render::DebugFlags;
    auto& flags = mContext.mDebugFlags;
    bool dirty = false;

    bool disabled = !flags.test(DebugFlags::eFactoryDebug);
    ImGui::BeginDisabled(disabled);
    ImGui::BeginGroup();
    dirty |= MyGui::CheckboxFlags<render::DebugFlags>("Debug layer", flags, DebugFlags::eDeviceDebugLayer);
    ImGui::SameLine();

    ImGui::BeginDisabled(!flags.test(DebugFlags::eDeviceDebugLayer));
    dirty |= MyGui::CheckboxFlags<render::DebugFlags>("Info queue", flags, DebugFlags::eInfoQueue);
    ImGui::SameLine();
    dirty |= MyGui::CheckboxFlags<render::DebugFlags>("GPU Validation", flags, DebugFlags::eGpuValidation);
    ImGui::EndDisabled();

    ImGui::SameLine();
    dirty |= MyGui::CheckboxFlags<render::DebugFlags>("DRED", flags, DebugFlags::eDeviceRemovedInfo);
    ImGui::EndGroup();
    ImGui::EndDisabled();
    if (disabled)
        ImGui::SetItemTooltip("Relaunch with --debug to enable");

    return dirty;
}

void RenderConfig::draw_content() {
    int backbuffers = int_cast<int>(mContext.mSwapChainConfig.length);

    if (ImGui::SliderInt("SwapChain Length", &backbuffers, 2, kMaxLength)) {
        mContext.update_swapchain_length(int_cast<uint>(backbuffers));
    }

    {
        auto [width, height] = mContext.mSwapChainConfig.size.as<int>();
        ImGui::Text("Display Resolution: %u x %u", width, height);
    }

    for (const auto& [camera, _] : mContext.cameras) {
        auto size = camera->config().size;
        ImGui::Text("Camera: %s (%u x %u)", camera->name().data(), size.width, size.height);
    }

    // {
    //     auto size = mContext.mSceneSize.as<int>();
    //     if (ImGui::SliderInt2("Render Resolution", size.data(), 64, 4096)) {
    //         mContext.update_scene_size(size.as<uint>());
    //     }
    // }

    ImGui::SeparatorText("Adapters");
    draw_adapters();

    ImGui::SeparatorText("Allocator");
    draw_allocator_info();

    ImGui::SeparatorText("Descriptor Heaps");
    auto& rtv = mContext.mRtvPool;
    auto& dsv = mContext.mDsvPool;
    auto& srv = mContext.mSrvPool;
    ImGui::Text("RTV: %u (%u used)", rtv.get_capacity(), rtv.get_used());
    ImGui::Text("DSV: %u (%u used)", dsv.get_capacity(), dsv.get_used());
    ImGui::Text("CBV/SRV/UAV: %u (%u used)", srv.get_capacity(), srv.get_used());

    ImGui::SeparatorText("Debug Flags");
    if (draw_debug_flags()) {
        mContext.set_device_lost();
    }
}

void RenderConfig::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin("Render Config", &mOpen)) {
        draw_content();
    }
    ImGui::End();
}
