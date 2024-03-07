#include "world/mesh.hpp"

using namespace sm;
using namespace sm::world;

Transform world::default_transform() {
    Transform result = {
        .position = 0.f,
        .rotation = radf3(0.f),
        .scale = 1.f
    };

    return result;
}

float4x4 Transform::matrix() const {
    return float4x4::transform(position, rotation, scale);
}

Mesh world::primitive(const MeshInfo&) {
    return {};
}
