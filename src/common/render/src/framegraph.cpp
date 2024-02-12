#include "render/framegraph.hpp"

using namespace sm::render;


void PassBuilder::write(const IGraphResource &resource) {
    (void)resource;
}

void PassBuilder::read(const IGraphResource &resource) {
    (void)resource;
}

void PassBuilder::side_effects() {
}
