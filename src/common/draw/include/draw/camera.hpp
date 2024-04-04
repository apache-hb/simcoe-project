#pragma once

#include "core/string.hpp"

#include "world/mesh.hpp"

#include "input/input.hpp"
#include "input/toggle.hpp"

#include "render/render.hpp"

#include <directx/dxgiformat.h>

namespace sm::draw {
    using namespace sm::math;

    struct ViewportConfig {
        math::uint2 size;
        DXGI_FORMAT colour;
        DXGI_FORMAT depth;

        float aspect_ratio() const { return float(size.width) / float(size.height); }
    };

    class Camera final : public input::IClient {
        sm::String mName;
        ViewportConfig mConfig;
        render::Viewport mViewport;

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
        Camera(sm::StringView name, const ViewportConfig& config);

        void tick(float dt);
        sm::StringView name() const;

        const ViewportConfig& config() const { return mConfig; }
        const render::Viewport& viewport() const { return mViewport; }

        // is the camera currently capturing input
        bool is_active() const;
        bool resize(const math::uint2& size);

        float4x4 model() const;
        float4x4 view() const;
        float4x4 projection(float aspect) const;

        float4x4 mvp(float aspect, const float4x4& object) const;
    };
}
