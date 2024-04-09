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

static auto& upload_light_data(
    graph::FrameGraph &graph,
    graph::Handle &point_light_data,
    graph::Handle &spot_light_data)
{
    graph::ResourceInfo point_light_info = {
        .sz = graph::ResourceSize::buffer(sizeof(LightVolumeData) * MAX_LIGHTS),
        .format = DXGI_FORMAT_UNKNOWN,
    };

    graph::ResourceInfo spot_light_info = {
        .sz = graph::ResourceSize::buffer(sizeof(LightVolumeData) * MAX_LIGHTS),
        .format = DXGI_FORMAT_UNKNOWN,
    };

    graph::PassBuilder pass = graph.copy("Upload Light Data");
    point_light_data = pass.create(point_light_info, "Point Light Data", graph::Access::eCopyTarget);
    spot_light_data = pass.create(spot_light_info, "Spot Light Data", graph::Access::eCopyTarget);

    auto& data = graph.device_data([](render::Context& context) {
        struct {
            SceneLightBuffer lights;

            render::Resource light_volume_upload;

            render::Resource point_light_upload;
            render::Resource spot_light_upload;
        } info;

        {
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightVolumeData) * MAX_LIGHTS);
            SM_ASSERT_HR(context.create_resource(info.light_volume_upload, D3D12_HEAP_TYPE_UPLOAD, desc, D3D12_RESOURCE_STATE_GENERIC_READ));
        }

        {
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(PointLightData) * MAX_POINT_LIGHTS);
            SM_ASSERT_HR(context.create_resource(info.point_light_upload, D3D12_HEAP_TYPE_UPLOAD, desc, D3D12_RESOURCE_STATE_GENERIC_READ));
        }

        {
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SpotLightData) * MAX_SPOT_LIGHTS);
            SM_ASSERT_HR(context.create_resource(info.spot_light_upload, D3D12_HEAP_TYPE_UPLOAD, desc, D3D12_RESOURCE_STATE_GENERIC_READ));
        }

        return info;
    });

    pass.bind([](graph::RenderContext& ctx) {

    });

    pass.side_effects(true);

    return data;
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

static void create_tiling_pipeline(render::Pipeline& pipeline, render::Context& context) {
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

        auto& device = context.mDevice;

        SM_ASSERT_HR(device->CreateComputePipelineState(&kDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

static void light_binning(
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
        .sz = graph::ResourceSize::buffer(LIGHT_INDEX_BUFFER_STRIDE * tile_count),
        .format = DXGI_FORMAT_UNKNOWN,
    };

    graph::PassBuilder pass = graph.compute(fmt::format("Forward+ Light Culling ({})", dd.camera.name()));
    pass.read(depth, "Depth", graph::Access::eDepthRead);
    indices = pass.create(info, "Light Indices", graph::Access::eUnorderedAccess);

    auto& data = graph.device_data([](render::Context& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        create_tiling_pipeline(info.pipeline, context);

        return info;
    });

    pass.bind([depth, indices, &data](graph::RenderContext& ctx) {
        auto& context = ctx.context;
        auto *cmd = ctx.commands;
        auto& graph = ctx.graph;

        auto dsv = graph.dsv(depth);
        auto uav = graph.uav(indices);

        auto dsv_gpu = context.mDsvPool.gpu_handle(dsv);
        auto uav_gpu = context.mSrvPool.gpu_handle(uav);

        cmd->SetComputeRootSignature(data.pipeline.signature.get());
        cmd->SetPipelineState(data.pipeline.pso.get());

        // TODO: everything else

        cmd->SetComputeRootDescriptorTable(eDepthTexture, dsv_gpu);

        cmd->SetComputeRootDescriptorTable(ePointLightData, uav_gpu);

        cmd->Dispatch(TILE_SIZE, TILE_SIZE, 1);
    });
}

void forward_plus::light_culling(
    graph::FrameGraph &graph,
    graph::Handle &indices,
    graph::Handle depth,
    DrawData dd)
{
    graph::Handle point_light_data;
    graph::Handle spot_light_data;

    upload_light_data(graph, point_light_data, spot_light_data);

    light_binning(graph, indices, depth, point_light_data, spot_light_data, dd);
}
