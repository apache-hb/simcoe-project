#pragma once

#include "render/draw.hpp"

#include "input/input.hpp"
#include "input/toggle.hpp"

#include "draw.reflect.h"

namespace sm::draw {
    class Camera final : public input::IClient {
        float3 mPosition = {-3.f, 0.f, 0.f};
        float3 mDirection = kVectorForward;

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

        void draw_debug();
        void tick(float dt);

        // is the camera currently capturing input
        bool is_active() const;
        Projection get_projection() const;

        float4x4 model() const;
        float4x4 view() const;
        float4x4 projection(float aspect) const;

        float4x4 mvp(float aspect, const float4x4& object) const;
    };
}
