#pragma once

#include "render/graph.hpp"
#include "render/render.hpp"

namespace sm::draw {
    class Camera;

    void opaque(graph::FrameGraph& graph, graph::Handle& target, const Camera& camera);
    void blit(graph::FrameGraph& graph, graph::Handle target, graph::Handle source, const render::Viewport& viewport);

    //void canvas(graph::FrameGraph& graph, graph::Handle target);
}
