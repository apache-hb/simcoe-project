#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

float4x4 Context::Camera::model() const {
    return float4x4::identity();
}

float4x4 Context::Camera::view() const {
    return float4x4::lookToRH(position, direction, kVectorUp);
}

float4x4 Context::Camera::projection(float aspect) const {
    return float4x4::perspectiveRH(fov, aspect, 0.1f, 100.f);
}

float4x4 Context::Camera::mvp(float aspect) const {
    return model() * view() * projection(aspect);
}
