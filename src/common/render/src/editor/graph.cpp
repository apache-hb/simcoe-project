#include "render/editor/graph.hpp"

#include "render/graph.hpp"
#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

void GraphPanel::draw_content() {
    ImGui::Text("Render Graph");
}

GraphPanel::GraphPanel(render::Context& context)
    : IEditorPanel("Render Graph")
    , mContext(context)
{ }
