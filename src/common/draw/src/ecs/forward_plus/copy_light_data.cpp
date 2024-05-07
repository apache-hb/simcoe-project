#include "stdafx.hpp"

#include "world/ecs.hpp"

#include "draw/draw.hpp"

using namespace sm;
using namespace sm::draw;
using namespace sm::world;

void draw::ecs::copyLightData(
    DrawData& dd,
    graph::Handle &spotLightVolumeData,
    graph::Handle &pointLightVolumeData,
    graph::Handle &spotLightData,
    graph::Handle &pointLightData
)
{
    const graph::ResourceInfo spotLightVolumeInfo = graph::ResourceInfo::structuredBuffer<LightVolumeData>(MAX_SPOT_LIGHTS).buffered();
    const graph::ResourceInfo pointLightVolumeInfo = graph::ResourceInfo::structuredBuffer<LightVolumeData>(MAX_POINT_LIGHTS).buffered();
    const graph::ResourceInfo spotLightInfo = graph::ResourceInfo::structuredBuffer<SpotLightData>(MAX_SPOT_LIGHTS).buffered();
    const graph::ResourceInfo pointLightInfo = graph::ResourceInfo::structuredBuffer<PointLightData>(MAX_POINT_LIGHTS).buffered();

    graph::PassBuilder pass = dd.graph.copy("Upload Light Data");

    spotLightVolumeData = pass.create(spotLightVolumeInfo, "Spot Light Volumes", graph::Usage::eCopyTarget);
    pointLightVolumeData = pass.create(pointLightVolumeInfo, "Point Light Volumes", graph::Usage::eCopyTarget);
    spotLightData = pass.create(spotLightInfo, "Spot Light Params", graph::Usage::eCopyTarget);
    pointLightData = pass.create(pointLightInfo, "Point Light Params", graph::Usage::eCopyTarget);

    auto& data = dd.graph.newDeviceData([](render::IDeviceContext& context) {
        struct {
            render::Resource pointLightVolumeData;
            render::Resource spotLightVolumeData;
            render::Resource pointLightData;
            render::Resource spotLightData;

            LightVolumeData *pointLightVolumeMemory;
            LightVolumeData *spotLightVolumeMemory;
            PointLightData *pointLightMemory;
            SpotLightData *spotLightMemory;

            LightVolumeData pointLightVolumeBuffer[kMaxPointLights] = {};
            LightVolumeData spotLightVolumeBuffer[kMaxSpotLights] = {};
            PointLightData pointLightBuffer[kMaxPointLights] = {};
            SpotLightData spotLightBuffer[kMaxSpotLights] = {};
        } info;

        auto createUploadBuffer = [&](render::Resource& resource, size_t size, void **memory) {
            SM_ASSERT_HR(context.createBufferResource(resource, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ));
            SM_ASSERT_HR(resource.map(nullptr, memory));
        };

        createUploadBuffer(info.pointLightVolumeData, sizeof(LightVolumeData) * kMaxPointLights, (void**)&info.pointLightVolumeMemory);
        createUploadBuffer(info.spotLightVolumeData, sizeof(LightVolumeData) * kMaxSpotLights, (void**)&info.spotLightVolumeMemory);
        createUploadBuffer(info.pointLightData, sizeof(PointLightData) * kMaxPointLights, (void**)&info.pointLightMemory);
        createUploadBuffer(info.spotLightData, sizeof(SpotLightData) * kMaxSpotLights, (void**)&info.spotLightMemory);

        return info;
    });

    pass.bind(
        [=, &data]
        (graph::RenderContext& ctx) {
        size_t pointLightIndex = 0;
        size_t spotLightIndex = 0;

        auto& [device, graph, _, commands] = ctx;

        gAllPointLights.iter(
            [&](
                flecs::iter& it,
                const world::ecs::Position* position,
                const world::ecs::Intensity* intensity,
                const world::ecs::Colour *colour
            ) {

            for (auto i : it) {
                size_t index = pointLightIndex++;
                LightVolumeData volume = {
                    .position = position[i].position,
                    .radius = intensity[i].intensity,
                };
                data.pointLightVolumeBuffer[index] = volume;

                PointLightData light = {
                    .colour = colour[i].colour
                };
                data.pointLightBuffer[index] = light;
            }
        });

        gAllSpotLights.iter(
            [&](
                flecs::iter& it,
                const world::ecs::Position* position,
                const world::ecs::Direction* direction,
                const world::ecs::Intensity* intensity,
                const world::ecs::Colour *colour
            ) {

            for (auto i : it) {
                size_t index = spotLightIndex++;
                LightVolumeData volume = {
                    .position = position[i].position,
                    .radius = intensity[i].intensity,
                };
                data.spotLightVolumeBuffer[index] = volume;

                SpotLightData light = {
                    .direction = direction[i].direction,
                    .colour = colour[i].colour,
                    .angle = 0.0f
                };
                data.spotLightBuffer[index] = light;
            }
        });

        ID3D12Resource *pointLightVolumeHandle = graph.resource(spotLightVolumeData);
        ID3D12Resource *pointLightHandle = graph.resource(pointLightData);
        ID3D12Resource *spotLightVolumeHandle = graph.resource(pointLightVolumeData);
        ID3D12Resource *spotLightHandle = graph.resource(spotLightData);

        memcpy(data.pointLightVolumeMemory, data.pointLightVolumeBuffer, sizeof(LightVolumeData) * pointLightIndex);
        memcpy(data.pointLightMemory, data.pointLightBuffer, sizeof(PointLightData) * pointLightIndex);

        memcpy(data.spotLightVolumeMemory, data.spotLightVolumeBuffer, sizeof(LightVolumeData) * spotLightIndex);
        memcpy(data.spotLightMemory, data.spotLightBuffer, sizeof(SpotLightData) * spotLightIndex);

        commands->CopyBufferRegion(pointLightVolumeHandle, 0, data.pointLightVolumeData.get(), 0, sizeof(LightVolumeData) * pointLightIndex);
        commands->CopyBufferRegion(pointLightHandle, 0, data.pointLightData.get(), 0, sizeof(PointLightData) * pointLightIndex);

        commands->CopyBufferRegion(spotLightVolumeHandle, 0, data.spotLightVolumeData.get(), 0, sizeof(LightVolumeData) * spotLightIndex);
        commands->CopyBufferRegion(spotLightHandle, 0, data.spotLightData.get(), 0, sizeof(SpotLightData) * spotLightIndex);
    });
}
