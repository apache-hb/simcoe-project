#pragma once

#include "render/graph.hpp"

namespace sm::draw {
    void opaque(graph::FrameGraph& graph, graph::Handle& target);
    void blit(graph::FrameGraph& graph, graph::Handle target, graph::Handle source);
    void canvas(graph::FrameGraph& graph, graph::Handle target);
}
