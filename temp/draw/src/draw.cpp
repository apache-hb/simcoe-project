#include "stdafx.hpp"

#include "draw/draw.hpp"

namespace ecs = sm::draw::ecs;
namespace host = sm::draw::host;

void host::CameraData::updateViewportData(ecs::ViewportDeviceData& data, uint pointLightCount, uint spotLightCount) const noexcept {
    float4x4 v = world::ecs::getViewMatrix(position, direction);
    float4x4 p = camera.getProjectionMatrix();

    data.update(draw::ViewportData {
        .viewProjection = float4x4::identity(),
        .worldView = v.transpose(),
        .projection = p.transpose(),
        .invProjection = p.inverse().transpose(),
        .cameraPosition = position,
        .windowSize = camera.window,
        .depthBufferSize = camera.window,
        .pointLightCount = pointLightCount,
        .spotLightCount = spotLightCount,
    });
}
