#include "stdafx.hpp"

#include "draw/camera.hpp"
#include "draw/common.hpp"
#include "draw/shared.hpp"

#include "draw/draw.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;
using namespace sm::draw;
using namespace sm::world;

struct SceneLightBuffer {
    sm::Vector<PointLightData> mPointLights;
    sm::Vector<SpotLightData> mSpotLights;

    sm::Vector<LightVolumeData> mPointLightVolumes;
    sm::Vector<LightVolumeData> mSpotLightVolumes;

    void update_light(World &world, IndexOf<Node> node, const float4x4& parent) {
        const Node& info = world.get(node);

        float4x4 model = parent * info.transform.matrix();

        for (IndexOf i : info.lights) {
            const Light& light = world.get(i);

            if (const world::PointLight *point = std::get_if<world::PointLight>(&light.light)) {
                LightVolumeData volume = {
                    .position = model.get_translation(),
                    .radius = point->intensity,
                };
                PointLightData data = {
                    .colour = point->colour,
                };

                mPointLights.push_back(data);
                mPointLightVolumes.push_back(volume);
            }
        }

        for (IndexOf i : info.children) {
            update_light(world, i, model);
        }
    }

    void update_lights(World &world, IndexOf<Node> root) {
        update_light(world, root, float4x4::identity());
    }
};

void forward_plus::upload_light_data(
    graph::FrameGraph &graph,
    graph::Handle &point_light_data,
    graph::Handle &spot_light_data)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
        .Buffer = {
            .FirstElement = 0,
            .NumElements = MAX_LIGHTS,
            .StructureByteStride = sizeof(LightVolumeData),
        },
    };

    graph::ResourceInfo light_data_info = {
        .size = graph::ResourceSize::buffer(sizeof(LightVolumeData) * MAX_LIGHTS),
        .format = DXGI_FORMAT_UNKNOWN,
    };

    graph::PassBuilder pass = graph.copy("Upload Light Data");

    point_light_data = pass.create(light_data_info, "Point Light Data", graph::Usage::eCopyTarget)
        .override_desc(uav);

    spot_light_data = pass.create(light_data_info, "Spot Light Data", graph::Usage::eCopyTarget)
        .override_desc(uav);

    graph.newDeviceData([](render::IDeviceContext& context) {
        struct {
            SceneLightBuffer lights;

            render::Resource light_volume_upload;

            render::Resource point_light_upload;
            render::Resource spot_light_upload;
        } info;

        {
            size_t size = sizeof(LightVolumeData) * MAX_LIGHTS;
            SM_ASSERT_HR(context.createBufferResource(info.light_volume_upload, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ));
        }

        {
            size_t size = sizeof(PointLightData) * MAX_POINT_LIGHTS;
            SM_ASSERT_HR(context.createBufferResource(info.point_light_upload, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ));
        }

        {
            size_t size = sizeof(SpotLightData) * MAX_SPOT_LIGHTS;
            SM_ASSERT_HR(context.createBufferResource(info.spot_light_upload, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ));
        }

        return info;
    });

    pass.bind([](graph::RenderContext& ctx) {

    });
}

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kComputeRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

enum Bindings {
    eFrameBuffer, // register(b1)

    ePointLightData, // register(t0)
    eSpotLightData, // register(t1)

    eDepthTexture, // register(t2)

    eLightIndexBuffer, // register(u0)

    eBindingCount
};

static void create_tiling_pipeline(render::Pipeline& pipeline, render::IDeviceContext& context) {
    {
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eFrameBuffer].InitAsConstantBufferView(0);

        params[ePointLightData].InitAsShaderResourceView(0);
        params[eSpotLightData].InitAsShaderResourceView(1);

        CD3DX12_DESCRIPTOR_RANGE1 depth[1];
        depth[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
        params[eDepthTexture].InitAsDescriptorTable(_countof(depth), depth);

        CD3DX12_DESCRIPTOR_RANGE1 indices[1];
        indices[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
        params[eLightIndexBuffer].InitAsDescriptorTable(_countof(indices), indices);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params, 0, nullptr, kComputeRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto cs = context.mConfig.bundle.get_shader_bytecode("forward_plus_tiling.cs");

        const D3D12_COMPUTE_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = pipeline.signature.get(),
            .CS = CD3DX12_SHADER_BYTECODE(cs.data(), cs.size()),
            .NodeMask = 0,
            .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
        };

        auto device = context.getDevice();

        SM_ASSERT_HR(device->CreateComputePipelineState(&kDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void forward_plus::light_binning(
    graph::FrameGraph &graph,
    graph::Handle &indices,
    graph::Handle depth,
    graph::Handle point_light_data,
    graph::Handle spot_light_data,
    forward_plus::DrawData dd)
{
    const auto& camera = dd.camera.config();
    uint tile_count = draw::get_tile_count(camera.size, TILE_SIZE);

    graph::ResourceInfo info = {
        .size = graph::ResourceSize::buffer(sizeof(uint) * LIGHT_INDEX_BUFFER_STRIDE * tile_count),
        .format = DXGI_FORMAT_R32_UINT,
    };

    graph::PassBuilder pass = graph.compute(fmt::format("Forward+ Light Binning ({})", dd.camera.name()));

    pass.read(depth, "Depth", graph::Usage::eTextureRead);
    pass.read(point_light_data, "Point Light Data", graph::Usage::eBufferRead);
    pass.read(spot_light_data, "Spot Light Data", graph::Usage::eBufferRead);

    indices = pass.create(info, "Light Indices", graph::Usage::eBufferWrite)
        .override_uav({
            .Format = DXGI_FORMAT_R32_UINT,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = LIGHT_INDEX_BUFFER_STRIDE * tile_count,
            },
        });

    auto& data = graph.newDeviceData([](render::IDeviceContext& context) {
        struct {
            render::Pipeline pipeline;

            render::ConstBuffer<draw::ViewportData> frame_buffer;
        } info;

        create_tiling_pipeline(info.pipeline, context);

        info.frame_buffer = render::newConstBuffer<draw::ViewportData>(context);

        return info;
    });

    pass.bind([depth, indices, pld = point_light_data, sld = spot_light_data, &data](graph::RenderContext& ctx) {
        auto& context = ctx.context;
        auto *cmd = ctx.commands;
        auto& graph = ctx.graph;

        auto dsv = graph.srv(depth);
        auto uav = graph.uav(indices);

        auto dsv_gpu = context.mSrvPool.gpu_handle(dsv);
        auto uav_gpu = context.mSrvPool.gpu_handle(uav);

        cmd->SetComputeRootSignature(data.pipeline.signature.get());
        cmd->SetPipelineState(data.pipeline.pso.get());

        cmd->SetComputeRootConstantBufferView(eFrameBuffer, data.frame_buffer.getDeviceAddress());

        cmd->SetComputeRootShaderResourceView(ePointLightData, graph.resource(pld)->GetGPUVirtualAddress());
        cmd->SetComputeRootShaderResourceView(eSpotLightData, graph.resource(sld)->GetGPUVirtualAddress());

        cmd->SetComputeRootDescriptorTable(eDepthTexture, dsv_gpu);

        cmd->SetComputeRootDescriptorTable(eLightIndexBuffer, uav_gpu);

        cmd->Dispatch(TILE_SIZE, TILE_SIZE, 1);
    });
}
