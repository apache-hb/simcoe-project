#include "render/camera.hpp"

#include "input/input.hpp"
#include "system/input.hpp"

#include "imgui/imgui.h"

using namespace sm;
using namespace sm::draw;
using namespace sm::input;

static constexpr ButtonAxis kMoveForward = {Button::eW, Button::eS};
static constexpr ButtonAxis kMoveStrafe = {Button::eD, Button::eA};
static constexpr ButtonAxis kMoveUp = {Button::eE, Button::eQ};

void Camera::draw_debug() {
    ImGui::SliderFloat("fov", &fov, 0.1f, 3.14f);
    ImGui::SliderFloat3("position", &mPosition.x, -10.f, 10.f);
    ImGui::SliderFloat3("direction", &mDirection.x, -1.f, 1.f);
    ImGui::SliderFloat("speed", &speed, 0.1f, 10.f);
    ImGui::SliderFloat("sensitivity", &sensitivity, 0.1f, 1.f);
}

void Camera::accept(const input::InputState& state, InputService& service) {
    constexpr auto key = input::Button::eTilde;
    if (mCameraActive.update(state.buttons[(size_t)key])) {
        service.capture_cursor(mCameraActive.is_active());
        sys::mouse::set_visible(!mCameraActive.is_active());
    }

    if (!mCameraActive.is_active()) {
        mMoveInput = {0.f, 0.f, 0.f};
        return;
    }

    float2 move = state.button_axis2d(kMoveStrafe, kMoveForward);
    float ascend = state.button_axis(kMoveUp);

    mMoveInput = {move.x, move.y, ascend};

    // do mouse input here, we get a new input state every frame
    // when the mouse is moving.
    float2 mouse = state.axis2d(Axis::eMouseX, Axis::eMouseY);

    mouse *= sensitivity;

    yaw += mouse.x;
    pitch += -mouse.y;

    pitch = math::clamp(pitch, -89.f, 89.f);

    float3 front = mDirection;
    front.x = std::cos(to_radians(pitch)) * std::cos(to_radians(yaw));
    front.y = std::cos(to_radians(pitch)) * std::sin(to_radians(yaw));
    front.z = -std::sin(to_radians(pitch));
    mDirection = front.normalized();
}

void Camera::tick(float dt) {
    if (!mCameraActive.is_active()) {
        return;
    }

    float scaled = speed * dt;

    // do keyboard input here, we only get a new input state
    // when a key is pressed or released.

    auto [x, y, z] = mMoveInput;
    float3 forward = mDirection;

    mPosition += forward * -y * scaled;
    mPosition += float3::cross(forward, kVectorUp).normalized() * -x * scaled;

    mPosition.z += -z * scaled;
}

float4x4 Camera::model() const {
    return float4x4::identity();
}

float4x4 Camera::view() const {
    return float4x4::lookToRH(mPosition, mDirection, kVectorUp);
}

float4x4 Camera::projection(float aspect) const {
    return float4x4::perspectiveRH(fov, aspect, 0.1f, 100.f);
}

float4x4 Camera::mvp(float aspect, const float4x4& object) const {
    return object * view() * projection(aspect);
}
