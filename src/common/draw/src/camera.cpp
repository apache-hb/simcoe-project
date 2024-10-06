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

Camera::Camera(sm::StringView name, const ViewportConfig& config)
    : mName(name)
    , mConfig(config)
    , mViewport(config.size)
{ }

void Camera::accept(const input::InputState& state, InputService& service) {
    constexpr auto key = input::Button::eTilde;
    if (mCameraActive.update(state.buttons[(size_t)key])) {
        service.capture_cursor(mCameraActive.is_active());
        system::mouse::set_visible(!mCameraActive.is_active());
    }

    if (!mCameraActive.is_active()) {
        mMoveInput = {0.f, 0.f, 0.f};
        return;
    }

    math::float2 move = state.button_axis2d(kMoveStrafe, kMoveForward);
    float ascend = state.button_axis(kMoveUp);

    mMoveInput = (move.x * world::kVectorForward) + (move.y * world::kVectorRight) + (ascend * world::kVectorUp);

    // do mouse input here, we get a new input state every frame
    // when the mouse is moving.
    math::float2 mouse = state.axis2d(Axis::eMouseX, Axis::eMouseY);

    mouse *= mMouseSensitivity;

    mLookYaw += -math::degf(mouse.x);
    mLookPitch += math::degf(mouse.y);

    mLookPitch = math::clamp(mLookPitch, -89._deg, 89._deg);

    math::float3 front = mDirection;
    front.x = math::cos(mLookPitch) * math::cos(mLookYaw);
    front.y = math::sin(mLookPitch);
    front.z = math::cos(mLookPitch) * math::sin(mLookYaw);
    mDirection = front.normalized();
}

void Camera::tick(float dt) {
    if (!mCameraActive.is_active())
        return;

    float scaled = mCameraSpeed * dt;

    // do keyboard input here, we only get a new input state
    // when a key is pressed or released.

    auto [x, y, z] = mMoveInput;
    math::float3 forward = mDirection;

    mPosition += forward * -y * scaled;
    mPosition += math::float3::cross(forward, world::kVectorUp).normalized() * -x * scaled;

    mPosition.z += -z * scaled;
}

sm::StringView Camera::name() const {
    return mName;
}

bool Camera::is_active() const {
    return mCameraActive.is_active();
}

bool Camera::resize(const math::uint2& size) {
    if (size == mConfig.size) return false;

    mConfig.size = size;
    mViewport = render::Viewport(size);

    return true;
}

math::float4x4 Camera::model() const {
    return math::float4x4::identity();
}

math::float4x4 Camera::view() const {
    return math::float4x4::lookToRH(mPosition, mDirection, world::kVectorUp);
}

math::float4x4 Camera::projection(float aspect) const {
    return math::float4x4::perspectiveRH(mFieldOfView, aspect, 0.1f, 100.f);
}

math::float4x4 Camera::mvp(float aspect, const math::float4x4& object) const {
    return object * view() * projection(aspect);
}
