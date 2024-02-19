#pragma once

#include "render/draw.hpp"

#include "input/input.hpp"
#include "input/toggle.hpp"

namespace sm::draw {
    class Camera final : public input::IClient {
        float3 mPosition = {3.f, 0.f, 0.f};
        float3 mDirection = -kVectorForward;

        float fov = to_radians(75.f);
        float speed = 3.f;
        float sensitivity = 0.1f;

        float pitch = 0.f;
        float yaw = 0.f;

        float3 mMoveInput = 0.f;

        void accept(const input::InputState& state) override;

        input::Toggle mCameraActive = false;

    public:
        Camera() = default;

        void update();
        void tick(float dt);

        float4x4 model() const;
        float4x4 view() const;
        float4x4 projection(float aspect) const;

        float4x4 mvp(float aspect, const float4x4& object) const;
    };
}
