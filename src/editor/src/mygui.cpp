#include "stdafx.hpp"

#include "editor/mygui.hpp"

using namespace sm;
using namespace sm::math;

bool MyGui::SliderAngle3(const char *label, radf3 *value, degf min, degf max) {
    auto deg = math::to_degrees(*value);
    if (ImGui::SliderFloat3(label, deg.data(), min.get_degrees(), max.get_degrees())) {
        *value = degf3(deg);
        return true;
    }
    return false;
}

bool MyGui::InputAngle3(const char *label, radf3 *value) {
    auto deg = math::to_degrees(*value);
    if (ImGui::InputFloat3(label, deg.data())) {
        *value = degf3(deg);
        return true;
    }
    return false;
}

bool MyGui::DragAngle3(const char *label, radf3 *value, degf speed, degf min, degf max) {
    auto deg = math::to_degrees(*value);
    if (ImGui::DragFloat3(label, deg.data(), speed.get_degrees(), min.get_degrees(), max.get_degrees())) {
        *value = degf3(deg);
        return true;
    }
    return false;
}

bool MyGui::EditSwizzle(const char *label, uint8 *mask, int components) {
    CTASSERTF(label != nullptr, "Invalid label");
    CTASSERTF(mask != nullptr, "Invalid mask");
    CTASSERTF(components <= 4, "Invalid number of components: %d", components);
    CTASSERTF(components > 0, "Invalid number of components: %d", components);

    constexpr const char *labels[] = { "X", "Y", "Z", "W" };
    constexpr ImGuiComboFlags flags = ImGuiComboFlags_NoArrowButton;

    ImGui::TextUnformatted(label);
    ImGui::PushID(label);
    ImGui::PushItemWidth(32.f);
    uint8 it = *mask;
    for (int i = 0; i < components; ++i) {
        ImGui::SameLine();
        int v = math::swizzle_get(it, Swizzle(i));
        ImGui::PushID(i);
        if (ImGui::BeginCombo("", labels[v], flags)) {
            for (int j = 0; j < 4; ++j) {
                bool selected = v == j;
                if (ImGui::Selectable(labels[j], selected)) {
                    v = j;
                    it = math::swizzle_set(it, Swizzle(i), Swizzle(v));
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
    }
    ImGui::PopItemWidth();
    ImGui::PopID();

    if (it != *mask) {
        *mask = it;
        return true;
    }

    return false;
}
