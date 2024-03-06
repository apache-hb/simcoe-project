#include "render/draw.hpp"

using namespace sm;
using namespace sm::draw;

float4x4 Transform::matrix() const {
    return float4x4::transform(position, rotation, scale);
}

float4x4 Camera::model() const {
    return float4x4::transform(position, {0, 0, 0}, {1, 1, 1});
}

float4x4 Camera::view() const {
    return float4x4::lookAtRH(position, position + forward, up);
}

float4x4 Camera::perspective(float width, float height) const {
    return float4x4::perspectiveRH(fov, width / height, near_plane, far_plane);
}
