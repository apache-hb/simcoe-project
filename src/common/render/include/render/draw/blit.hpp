#pragma once

#include "render/graph.hpp"

namespace sm::draw {
    void blit_texture(graph::FrameGraph& graph, graph::Handle target, graph::Handle source);
}
