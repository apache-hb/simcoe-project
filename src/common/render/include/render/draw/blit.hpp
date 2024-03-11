#pragma once

#include "render/graph.hpp"

namespace sm::draw {
    void blit(graph::FrameGraph& graph, graph::Handle target, graph::Handle source);
}
