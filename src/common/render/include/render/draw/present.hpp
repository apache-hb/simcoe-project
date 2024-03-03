#pragma once

#include "render/graph.hpp"

namespace sm::draw {
    void draw_present(graph::FrameGraph& graph, graph::Handle target, graph::Handle source);
}
