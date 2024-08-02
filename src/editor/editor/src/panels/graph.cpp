#include "stdafx.hpp"

#include "editor/panels/graph.hpp"

#include "render/core/graph.hpp"
#include "render/core/render.hpp"

using namespace sm;
using namespace sm::ed;

void GraphPanel::draw_graph() {

}

void GraphPanel::draw_lifetimes() {

}

void GraphPanel::draw_content() {
    if (ImGui::BeginTabBar("GraphTabs")) {
        if (ImGui::BeginTabItem("Graph")) {
            draw_graph();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lifetimes")) {
            draw_lifetimes();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

GraphPanel::GraphPanel(render::IDeviceContext& context)
    : IEditorPanel("Render Graph")
    , mContext(context)
{
    mFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
}
