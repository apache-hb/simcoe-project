#include "stdafx.hpp"

#include "draw/camera.hpp"

#include "input/input.hpp"
#include "system/input.hpp"

using namespace sm;
using namespace sm::draw;
using namespace sm::input;

static constexpr ButtonAxis kMoveForward = {Button::eW, Button::eS};
static constexpr ButtonAxis kMoveStrafe = {Button::eD, Button::eA};
static constexpr ButtonAxis kMoveUp = {Button::eE, Button::eQ};

Camera::Camera(sm::StringView name)
    : mName(name)
{ }

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

    mouse *= mMouseSensitivity;

    mLookYaw += degf(mouse.x);
    mLookPitch += -degf(mouse.y);

    mLookPitch = math::clamp(mLookPitch, -89._deg, 89._deg);

    float3 front = mDirection;
    front.x = math::cos(mLookPitch) * math::cos(mLookYaw);
    front.y = math::cos(mLookPitch) * math::sin(mLookYaw);
    front.z = -math::sin(mLookPitch);
    mDirection = front.normalized();
}

void Camera::tick(float dt) {
    if (!mCameraActive.is_active()) {
        return;
    }

    float scaled = mCameraSpeed * dt;

    // do keyboard input here, we only get a new input state
    // when a key is pressed or released.

    auto [x, y, z] = mMoveInput;
    float3 forward = mDirection;

    mPosition += forward * -y * scaled;
    mPosition += float3::cross(forward, world::kVectorUp).normalized() * -x * scaled;

    mPosition.z += -z * scaled;
}

sm::StringView Camera::name() const {
    return mName;
}

bool Camera::is_active() const {
    return mCameraActive.is_active();
}

float4x4 Camera::model() const {
    return float4x4::identity();
}

float4x4 Camera::view() const {
    return float4x4::lookToRH(mPosition, mDirection, world::kVectorUp);
}

float4x4 Camera::projection(float aspect) const {
    return float4x4::perspectiveRH(mFieldOfView, aspect, 0.1f, 100.f);
}

float4x4 Camera::mvp(float aspect, const float4x4& object) const {
    return object * view() * projection(aspect);
}
