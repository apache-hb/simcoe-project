#include "world/mesh.hpp"

using namespace sm;
using namespace sm::world;

Transform world::default_transform() {
    Transform result = {
        .position = {0.f, 0.f, 0.f},
        .rotation = quatf::identity(),
        .scale = 1.f
    };

    return result;
}

Mesh world::primitive(const MeshInfo&) {
    return {};
}
