#include "render/camera.hpp"

#include "imgui/imgui.h"

using namespace sm;
using namespace sm::draw;

void Camera::update() {
    ImGui::SliderFloat("fov", &fov, 0.1f, 3.14f);
    ImGui::SliderFloat3("position", &position.x, -10.f, 10.f);
    ImGui::SliderFloat3("direction", &direction.x, -1.f, 1.f);
    ImGui::SliderFloat("speed", &speed, 0.1f, 10.f);
}

void Camera::accept(const input::InputState& state) {

}

float4x4 Camera::model() const {
    return float4x4::identity();
}

float4x4 Camera::view() const {
    return float4x4::lookAtRH(position, direction, kVectorUp);
}

float4x4 Camera::projection(float aspect) const {
    return float4x4::perspectiveRH(fov, aspect, 0.1f, 100.f);
}

float4x4 Camera::mvp(float aspect, const float4x4& object) const {
    return object * view() * projection(aspect);
}
