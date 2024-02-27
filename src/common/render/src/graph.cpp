#include "render/graph.hpp"

#include "core/map.hpp"
#include "core/stack.hpp"

using namespace sm;
using namespace sm::graph;

void FrameGraph::add(IRenderPass *pass) {
    PassBuilder builder{mContext, *pass, *this};
    pass->build(builder);
    mPasses.push_back(pass);
}
