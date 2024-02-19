#pragma once

#include "render/draw.hpp"

#include "input/input.hpp"

namespace sm::draw {
    class Camera final : input::IClient {
        float3 position = {3.f, 0.f, 0.f};
        float3 direction = kVectorForward;

        float fov = to_radians(75.f);
        float speed = 3.f;

        void accept(const input::InputState& state) override;

    public:
        Camera() = default;

        void update();

        float4x4 model() const;
        float4x4 view() const;
        float4x4 projection(float aspect) const;

        float4x4 mvp(float aspect, const float4x4& object) const;
    };
}
