#pragma once

#include "core/string.hpp"

#include "world/mesh.hpp"
#include "world/world.hpp"

#include "input/input.hpp"
#include "input/toggle.hpp"

#include "render/render.hpp"

#include <directx/dxgiformat.h>

namespace sm::draw {
    using namespace sm::math::literals;

    struct ViewportConfig {
        math::uint2 size;
        DXGI_FORMAT colour;
        DXGI_FORMAT depth;

        float getAspectRatio() const { return float(size.width) / float(size.height); }
        DXGI_FORMAT getDepthFormat() const { return depth; }
        DXGI_FORMAT getColourFormat() const { return colour; }
    };

    class Camera final : public input::IClient {

        sm::String mName;
        ViewportConfig mConfig;
        render::Viewport mViewport;
        world::IndexOf<world::Camera> mCameraIndex;

        math::float3 mPosition = {-3.f, 0.f, 0.f};
        math::float3 mDirection = world::kVectorForward;

        math::radf mFieldOfView = 75._deg;
        float mCameraSpeed = 3.f;
        float mMouseSensitivity = 0.1f;

        math::degf mLookPitch = 0._deg;
        math::degf mLookYaw = 0._deg;

        math::float3 mMoveInput = 0.f;

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

        math::float4x4 model() const;
        math::float4x4 view() const;
        math::float4x4 projection(float aspect) const;

        math::float4x4 mvp(float aspect, const math::float4x4& object) const;

        void setPosition(const math::float3& position) { mPosition = position; }
        void setDirection(const math::float3& direction) { mDirection = direction; }

        math::float3 position() const { return mPosition; }
        math::float3 direction() const { return mDirection; }
    };
}
