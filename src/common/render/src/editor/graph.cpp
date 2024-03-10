#include "render/editor/graph.hpp"

#include "render/graph.hpp"
#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

void GraphPanel::draw_graph() {
    ImNodes::BeginCanvas(&mCanvasState);

    for (auto& pass : mContext.mFrameGraph.mRenderPasses) {
        if (ImNodes::Ez::BeginNode(&pass, pass.name.c_str(), nullptr, nullptr)) {
            sm::SmallVector<ImNodes::Ez::SlotInfo, 4> inputs;
            sm::SmallVector<ImNodes::Ez::SlotInfo, 4> outputs;

            for (auto& _ : pass.reads) {
                inputs.push_back({ "read", 1 });
            }

            for (auto& _ : pass.writes) {
                outputs.push_back({ "write", 1 });
            }

            for (auto& _ : pass.creates) {
                outputs.push_back({ "create", 1 });
            }

            ImNodes::Ez::InputSlots(inputs.data(), inputs.size());
            ImNodes::Ez::OutputSlots(outputs.data(), outputs.size());
            ImNodes::Ez::EndNode();
        }
    }

    ImNodes::EndCanvas();
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
