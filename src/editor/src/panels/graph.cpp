#include "stdafx.hpp"

#include "editor/graph.hpp"

#include "render/graph.hpp"
#include "render/render.hpp"

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

GraphPanel::GraphPanel(render::Context& context)
    : IEditorPanel("Render Graph")
    , mContext(context)
{
    mFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
}
