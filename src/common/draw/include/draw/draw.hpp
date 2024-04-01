#pragma once

#include "render/graph.hpp"

namespace sm::draw {
    class Camera;

    void opaque(graph::FrameGraph& graph, graph::Handle& target, const Camera& camera);
    void blit(graph::FrameGraph& graph, graph::Handle target, graph::Handle source);
    void canvas(graph::FrameGraph& graph, graph::Handle target);
}
