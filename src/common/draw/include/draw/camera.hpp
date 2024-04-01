#pragma once

#include "world/mesh.hpp"

#include "input/input.hpp"
#include "input/toggle.hpp"

namespace sm::draw {
    using namespace sm::math;

    class Camera final : public input::IClient {
        float3 mPosition = {-3.f, 0.f, 0.f};
        float3 mDirection = world::kVectorForward;

        radf mFieldOfView = 75._deg;
        float mCameraSpeed = 3.f;
        float mMouseSensitivity = 0.1f;

        degf mLookPitch = 0._deg;
        degf mLookYaw = 0._deg;

        float3 mMoveInput = 0.f;

        void accept(const input::InputState& state, input::InputService& service) override;

        input::Toggle mCameraActive = false;

    public:
        Camera() = default;

        void tick(float dt);

        // is the camera currently capturing input
        bool is_active() const;

        float4x4 model() const;
        float4x4 view() const;
        float4x4 projection(float aspect) const;

        float4x4 mvp(float aspect, const float4x4& object) const;
    };
}
