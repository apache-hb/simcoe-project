#include "stdafx.hpp"

#include "world/ecs.hpp"

#include "draw/draw.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;
using namespace sm::draw;
using namespace sm::world;

void draw::ecs::copyLightData(
    flecs::world& world,
    graph::FrameGraph &graph,
    graph::Handle &spotLightVolumeData,
    graph::Handle &pointLightVolumeData,
    graph::Handle &spotLightData,
    graph::Handle &pointLightData)
{
    static flecs::query queryAllPointLights
        = world.query_builder<
            const world::ecs::Position,
            const world::ecs::Direction,
            const world::ecs::Intensity,
            const world::ecs::Colour
        >()
        // select the world position
        .term_at(1).second<world::ecs::World>()
        // only select point lights
        .with<world::ecs::PointLight>()
        .build();

    static flecs::query queryAllSpotLights
        = world.query_builder<
            const world::ecs::Position,
            const world::ecs::Direction,
            const world::ecs::Intensity,
            const world::ecs::Colour
        >()
        // select the world position
        .term_at(1).second<world::ecs::World>()
        // only select spot lights
        .with<world::ecs::SpotLight>()
        .build();

    const graph::ResourceInfo lightDataInfo = {
        .size = graph::ResourceSize::buffer(sizeof(LightVolumeData) * MAX_LIGHTS)
    };

    graph::PassBuilder pass = graph.copy("Upload Light Data");

    spotLightVolumeData = pass.create(lightDataInfo, "Spot Light Volume Data", graph::Usage::eCopyTarget)
        .override_uav({
            .Format = DXGI_FORMAT_UNKNOWN,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = MAX_SPOT_LIGHTS,
                .StructureByteStride = sizeof(LightVolumeData),
            },
        });

    pointLightVolumeData = pass.create(lightDataInfo, "Point Light Volume Data", graph::Usage::eCopyTarget)
        .override_uav({
            .Format = DXGI_FORMAT_UNKNOWN,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = MAX_POINT_LIGHTS,
                .StructureByteStride = sizeof(LightVolumeData),
            },
        });

    spotLightData = pass.create(lightDataInfo, "Spot Light Data", graph::Usage::eCopyTarget)
        .override_uav({
            .Format = DXGI_FORMAT_UNKNOWN,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = MAX_SPOT_LIGHTS,
                .StructureByteStride = sizeof(SpotLightData),
            },
        });

    pointLightData = pass.create(lightDataInfo, "Point Light Data", graph::Usage::eCopyTarget)
        .override_uav({
            .Format = DXGI_FORMAT_UNKNOWN,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = MAX_POINT_LIGHTS,
                .StructureByteStride = sizeof(PointLightData),
            },
        });

    auto& data = graph.device_data([](render::IDeviceContext& context) {
        struct {
            render::Resource pointLightVolumeData;
            render::Resource spotLightVolumeData;
            render::Resource pointLightData;
            render::Resource spotLightData;

            LightVolumeData *pointLightVolumeMemory;
            LightVolumeData *spotLightVolumeMemory;
            PointLightData *pointLightMemory;
            SpotLightData *spotLightMemory;

            LightVolumeData pointLightVolumeBuffer[MAX_POINT_LIGHTS] = {};
            LightVolumeData spotLightVolumeBuffer[MAX_SPOT_LIGHTS] = {};
            PointLightData pointLightBuffer[MAX_POINT_LIGHTS] = {};
            SpotLightData spotLightBuffer[MAX_SPOT_LIGHTS] = {};
        } info;

        {
            size_t size = sizeof(LightVolumeData) * MAX_POINT_LIGHTS;
            SM_ASSERT_HR(context.createBufferResource(info.pointLightVolumeData, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ));

            SM_ASSERT_HR(info.pointLightVolumeData.map(nullptr, (void**)&info.pointLightVolumeMemory));
        }

        {
            size_t size = sizeof(LightVolumeData) * MAX_SPOT_LIGHTS;
            SM_ASSERT_HR(context.createBufferResource(info.spotLightVolumeData, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ));

            SM_ASSERT_HR(info.spotLightVolumeData.map(nullptr, (void**)&info.spotLightVolumeMemory));
        }

        {
            size_t size = sizeof(PointLightData) * MAX_POINT_LIGHTS;
            SM_ASSERT_HR(context.createBufferResource(info.pointLightData, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ));

            SM_ASSERT_HR(info.pointLightData.map(nullptr, (void**)&info.pointLightMemory));
        }

        {
            size_t size = sizeof(SpotLightData) * MAX_SPOT_LIGHTS;
            SM_ASSERT_HR(context.createBufferResource(info.spotLightData, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ));

            SM_ASSERT_HR(info.spotLightData.map(nullptr, (void**)&info.spotLightMemory));
        }

        return info;
    });

    pass.bind([=, &data](graph::RenderContext& ctx) {
        size_t pointLightIndex = 0;
        size_t spotLightIndex = 0;

        auto& [device, graph, _, commands] = ctx;


        if (queryAllPointLights.changed()) {
            queryAllPointLights.iter(
                [&](
                    flecs::iter& it,
                    const world::ecs::Position* position,
                    const world::ecs::Direction* direction,
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

            ID3D12Resource *pointLightVolumeHandle = graph.resource(spotLightVolumeData);
            ID3D12Resource *pointLightHandle = graph.resource(pointLightData);

            memcpy(data.pointLightVolumeMemory, data.pointLightVolumeBuffer, sizeof(LightVolumeData) * pointLightIndex);
            memcpy(data.pointLightMemory, data.pointLightBuffer, sizeof(PointLightData) * pointLightIndex);

            commands->CopyBufferRegion(pointLightVolumeHandle, 0, data.pointLightVolumeData.get(), 0, sizeof(LightVolumeData) * pointLightIndex);
            commands->CopyBufferRegion(pointLightHandle, 0, data.pointLightData.get(), 0, sizeof(PointLightData) * pointLightIndex);
        }

        queryAllSpotLights.iter(
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

            ID3D12Resource *spotLightVolumeHandle = graph.resource(pointLightVolumeData);
            ID3D12Resource *spotLightHandle = graph.resource(spotLightData);

            memcpy(data.spotLightVolumeMemory, data.spotLightVolumeBuffer, sizeof(LightVolumeData) * spotLightIndex);
            memcpy(data.spotLightMemory, data.spotLightBuffer, sizeof(SpotLightData) * spotLightIndex);

            commands->CopyBufferRegion(spotLightVolumeHandle, 0, data.spotLightVolumeData.get(), 0, sizeof(LightVolumeData) * spotLightIndex);
            commands->CopyBufferRegion(spotLightHandle, 0, data.spotLightData.get(), 0, sizeof(SpotLightData) * spotLightIndex);
        });

    });
}