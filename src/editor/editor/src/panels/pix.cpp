#include "stdafx.hpp"

#include "editor/panels/pix.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::ed;

bool PixPanel::is_pix_enabled() const {
    return mContext.mDebugFlags.test(render::DebugFlags::eWinPixEventRuntime);
}

void PixPanel::draw_content() {
    auto val = std::to_underlying(mOptions);
    bool changed = false;

    bool disabled = !is_pix_enabled();
    ImGui::BeginDisabled(disabled);
    ImGui::BeginGroup();

    changed |= ImGui::RadioButton("Show on all windows", &val, PIX_HUD_SHOW_ON_ALL_WINDOWS);
    ImGui::SameLine();
    changed |= ImGui::RadioButton("Show on target window", &val, PIX_HUD_SHOW_ON_TARGET_WINDOW_ONLY);
    ImGui::SameLine();
    changed |= ImGui::RadioButton("Show on no windows", &val, PIX_HUD_SHOW_ON_NO_WINDOWS);

    ImGui::EndGroup();
    ImGui::EndDisabled();

    if (disabled)
        ImGui::SetItemTooltip("Relaunch with --pix to enable");

    if (changed) {
        mOptions = static_cast<PIXHUDOptions>(val);
        PIXSetHUDOptions(mOptions);
    }
}

bool PixPanel::draw_menu_item(const char *shortcut) {
    bool disabled = !is_pix_enabled();
    ImGui::MenuItem("PIX", shortcut, &mOpen, !disabled);
    if (disabled)
        ImGui::SetItemTooltip("Relaunch with --pix to enable");

    return false;
}

PixPanel::PixPanel(render::Context& context)
    : mContext(context)
{
    mOpen = is_pix_enabled();
}

void PixPanel::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin("PIX", &mOpen)) {
        draw_content();
    }
    ImGui::End();
}
