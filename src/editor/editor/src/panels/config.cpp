#include "stdafx.hpp"

#include "editor/panels/config.hpp"

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

static bool draw_adapter_info(render::IDeviceContext& context, const render::Adapter& adapter) {
    bool result = false;
    auto luid = adapter.luid();
    std::string label = fmt::format("##{}{}", luid.high, luid.low);

    if (ImGui::RadioButton(label.c_str(), context.get_current_adapter() == adapter)) {
        result = true;
    }
    ImGui::SameLine();

    auto name = adapter.name();

    if (ImGui::TreeNodeEx((void *)name.data(), ImGuiTreeNodeFlags_None, "%s",
                            name.data())) {
        ImGui::Text("Video memory: %s", adapter.vidmem().toString().c_str());
        ImGui::Text("System memory: %s", adapter.sysmem().toString().c_str());
        ImGui::Text("Shared memory: %s", adapter.sharedmem().toString().c_str());
        ImGui::Text("Flags: %s", ctu::TypeInfo<render::AdapterFlag>::to_string(adapter.flags()).data());
        ImGui::Text("LUID: %08lX:%08X", luid.high, luid.low);
        ImGui::SameLine();
        if (ImGui::SmallButton("Copy##LUID")) {
            auto arg = fmt::format("--adapter {:08x}:{:08x}", luid.high, luid.low);
            ImGui::SetClipboardText(arg.c_str());
        }
        auto info = adapter.info();
        ImGui::Text("Vendor: %04X Device: %04X Subsystem: %04X Revision: %04X",
                    info.vendor, info.device, info.subsystem, info.revision);
        ImGui::TreePop();
    }

    return result;
}

void RenderConfig::draw_adapters() const {
    auto& instance = mContext.mInstance;
    auto& warp = instance.get_warp_adapter();

    if (draw_adapter_info(mContext, warp)) {
        mContext.markDeviceDirty();
        mContext.set_current_adapter(warp);
    }

    for (auto& adapter : instance.get_adapters()) {
        if (adapter == instance.get_warp_adapter()) continue;

        if (draw_adapter_info(mContext, adapter)) {
            mContext.markDeviceDirty();
            mContext.set_current_adapter(adapter);
        }
    }
}

static void display_mem_budget(const D3D12MA::Budget &budget) {
    sm::Memory usage_bytes = budget.UsageBytes;
    sm::Memory budget_bytes = budget.BudgetBytes;
    ImGui::Text("Usage: %s", usage_bytes.toString().c_str());
    ImGui::Text("Budget: %s", budget_bytes.toString().c_str());

    uint64 alloc_count = budget.Stats.AllocationCount;
    sm::Memory alloc_bytes = budget.Stats.AllocationBytes;
    uint64 block_count = budget.Stats.BlockCount;
    sm::Memory block_bytes = budget.Stats.BlockBytes;

    ImGui::Text("Allocated blocks: %llu", alloc_count);
    ImGui::Text("Allocated: %s", alloc_bytes.toString().c_str());

    ImGui::Text("Block count: %llu", block_count);
    ImGui::Text("Block: %s", block_bytes.toString().c_str());
}

void RenderConfig::draw_allocator_info() const {
    D3D12MA::Allocator *allocator = mContext.getAllocator();

    {
        sm::Memory local = allocator->GetMemoryCapacity(DXGI_MEMORY_SEGMENT_GROUP_LOCAL);
        sm::Memory nonlocal = allocator->GetMemoryCapacity(DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL);

        ImGui::Text("Local: %s", local.toString().c_str());
        ImGui::Text("Non-Local: %s", nonlocal.toString().c_str());
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
    int backbuffers = int_cast<int>(mContext.getSwapChainLength());

    if (ImGui::SliderInt("SwapChain Length", &backbuffers, 2, kMaxLength)) {
        mContext.update_swapchain_length(int_cast<uint>(backbuffers));
    }

    {
        auto [width, height] = math::int2(mContext.getSwapChainSize());
        ImGui::Text("Display Resolution: %u x %u", width, height);
    }

#if 0
    for (const auto& data : mContext.get_cameras()) {
        auto& camera = data->camera;
        auto size = camera.config().size;
        ImGui::Text("Camera: %s (%u x %u)", camera.name().data(), size.width, size.height);
    }
#endif

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
    ImGui::Text("RTV: %u (%u used)", rtv.getCapacity(), rtv.countUsedHandles());
    ImGui::Text("DSV: %u (%u used)", dsv.getCapacity(), dsv.countUsedHandles());
    ImGui::Text("CBV/SRV/UAV: %u (%u used)", srv.getCapacity(), srv.countUsedHandles());

    ImGui::SeparatorText("Debug Flags");
    if (draw_debug_flags()) {
        mContext.markDeviceDirty();
    }
}

void RenderConfig::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin("Render Config", &mOpen)) {
        draw_content();
    }
    ImGui::End();
}
