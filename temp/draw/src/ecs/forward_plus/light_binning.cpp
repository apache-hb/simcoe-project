#include "stdafx.hpp"

#include "draw/draw.hpp"

#include "world/ecs.hpp"

using namespace sm;
using namespace sm::draw;

enum {
    eViewportBuffer, // register(b0)

    eDispatchInfo, // register(b1)

    ePointLightData, // register(t0)
    eSpotLightData, // register(t1)

    eDepthTexture, // register(t2)

    eLightIndexBuffer, // register(u0)

    eBindingCount
};

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kBinningPassRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

static void createLightBinningPipeline(
    render::Pipeline& pipeline,
    render::IDeviceContext& context
)
{
    {
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eViewportBuffer].InitAsConstantBufferView(0);
        params[eDispatchInfo].InitAsConstants(3, 1, 0, D3D12_SHADER_VISIBILITY_ALL);

        params[ePointLightData].InitAsShaderResourceView(0);
        params[eSpotLightData].InitAsShaderResourceView(1);

        CD3DX12_DESCRIPTOR_RANGE1 depth[1];
        depth[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

        params[eDepthTexture].InitAsDescriptorTable(_countof(depth), depth);

        CD3DX12_DESCRIPTOR_RANGE1 indices[1];
        indices[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

        params[eLightIndexBuffer].InitAsDescriptorTable(_countof(indices), indices);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params, 0, nullptr, kBinningPassRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto cs = context.mConfig.bundle->get_shader_bytecode("forward_plus_tiling.cs");

        const D3D12_COMPUTE_PIPELINE_STATE_DESC kPipeline = {
            .pRootSignature = pipeline.signature.get(),
            .CS = CD3DX12_SHADER_BYTECODE(cs.data(), cs.size()),
        };

        auto device = context.getDevice();
        SM_ASSERT_HR(device->CreateComputePipelineState(&kPipeline, IID_PPV_ARGS(&pipeline.pso)));
    }

    pipeline.rename("forward_plus_tiling");
}

void ecs::lightBinning(
    WorldData& wd,
    DrawData& dd,
    graph::Handle &indices,
    graph::Handle depth,
    graph::Handle pointLightData,
    graph::Handle spotLightData
)
{
    const world::ecs::Camera& info = wd.camera;

    uint tileIndexCount = getTileIndexCount(info.window, TILE_SIZE);
    uint2 gridSize = computeGridSize(info.window, TILE_SIZE);

    LOG_INFO(DrawLog,
        "window size: {}, tile index count: {}, grid size: {}, max grid index: {}",
        info.window, tileIndexCount,
        gridSize, (gridSize.x * gridSize.y) * LIGHT_INDEX_BUFFER_STRIDE
    );

    const graph::ResourceInfo lightIndexInfo = graph::ResourceInfo::arrayOf<light_index_t>(tileIndexCount);

    graph::PassBuilder pass = dd.graph.compute("Light Binning");

    indices = pass.create(lightIndexInfo, "Light Indices", graph::Usage::eBufferWrite)
        .override_srv({
            .Format = render::bufferFormatOf<light_index_t>(),
            .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = tileIndexCount,
            },
        });

    pass.read(depth, "Depth", graph::Usage::eTextureRead);
    pass.read(pointLightData, "Point Light Volumes", graph::Usage::eBufferRead);
    pass.read(spotLightData, "Spot Light Volumes", graph::Usage::eBufferRead);

    auto& data = dd.graph.newDeviceData([](render::IDeviceContext& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        createLightBinningPipeline(info.pipeline, context);

        return info;
    });

    pass.bind([=, vpd = &wd.viewport, &data](graph::RenderContext& ctx) {
        auto& [context, graph, _, commands] = ctx;

        ID3D12Resource *pointLightHandle = graph.resource(pointLightData);
        ID3D12Resource *spotLightHandle = graph.resource(spotLightData);

        auto depthTexture = graph.srv(depth);
        auto depthTextureHandle = context.mSrvPool.gpu_handle(depthTexture);

        auto lightIndices = graph.uav(indices);
        auto lightIndicesHandle = context.mSrvPool.gpu_handle(lightIndices);

        // LOG_INFO(GlobalLog, "window size: {}", ((ViewportData*)vpd->mapped)->windowSize);

        commands->SetComputeRootSignature(data.pipeline.signature.get());
        commands->SetPipelineState(data.pipeline.pso.get());

        commands->SetComputeRootConstantBufferView(eViewportBuffer, vpd->getDeviceAddress());

        UINT32 dispatchInfo[] = { gridSize.x, gridSize.y, tileIndexCount };
        commands->SetComputeRoot32BitConstants(eDispatchInfo, sizeof(dispatchInfo) / sizeof(UINT32), dispatchInfo, 0);

        commands->SetComputeRootShaderResourceView(ePointLightData, pointLightHandle->GetGPUVirtualAddress());
        commands->SetComputeRootShaderResourceView(eSpotLightData, spotLightHandle->GetGPUVirtualAddress());

        commands->SetComputeRootDescriptorTable(eDepthTexture, depthTextureHandle);

        commands->SetComputeRootDescriptorTable(eLightIndexBuffer, lightIndicesHandle);

        LOG_INFO(DrawLog, "dispatching {}x{}x1 for tile buffer ({})", gridSize.x, gridSize.y, tileIndexCount);
        commands->Dispatch(gridSize.x, gridSize.y, 1);
    });
}

#ifdef __cplusplus
// simple case:
// - window size: (1920, 1080)
// - tile size: 16
constexpr void test1() {
    static constexpr uint2 kWindowSize = uint2(1920, 1080);
    static constexpr uint kTileSize = 16;

    static constexpr uint2 kExpectedGridSize = uint2(120, 68);
    static constexpr uint kExpectedTileCount = 8160;

    static_assert(computeGridSize(kWindowSize, kTileSize) == uint2(120, 68));
    static_assert(computeTileCount(kWindowSize, kTileSize) == kExpectedTileCount);

    static_assert(computeWindowTiledSize(kWindowSize, kTileSize) == kExpectedGridSize * kTileSize);

    static_assert(computePixelTileIndex(kWindowSize, float2(0, 0), kTileSize) == 0);
    static_assert(computePixelTileIndex(kWindowSize, float2(kWindowSize), kTileSize) == kExpectedTileCount);

    static_assert(computeGroupTileIndex(uint3(0, 0, 0), kWindowSize, kTileSize) == 0);
    static_assert(computeGroupTileIndex(uint3(1, 0, 0), kWindowSize, kTileSize) == 1);
    static_assert(computeGroupTileIndex(uint3(0, 1, 0), kWindowSize, kTileSize) == kExpectedGridSize.x);
    static_assert(computeGroupTileIndex(uint3(16, 16, 0), kWindowSize, kTileSize) == 1936);

    // TODO: the <= comparison is technically wrong, it means we may skip the bottom right tile
    static_assert(computeGroupTileIndex(uint3(kExpectedGridSize - 1, 0), kWindowSize, kTileSize) <= kExpectedTileCount);
};

// case 1:
// - window size: (2119, 860)
// - tile size: 16
constexpr void test2() {
    static constexpr uint2 kWindowSize = uint2(2119, 860);
    static constexpr uint kTileSize = 16;

    static constexpr uint2 kExpectedGridSize = uint2(133, 54);
    static constexpr uint kExpectedTileCount = 7182;

    static_assert(computeGridSize(kWindowSize, kTileSize) == kExpectedGridSize);
    static_assert(computeTileCount(kWindowSize, kTileSize) == kExpectedTileCount);

    static_assert(computeWindowTiledSize(kWindowSize, kTileSize) == kExpectedGridSize * kTileSize);

    static_assert(computePixelTileIndex(kWindowSize, float2(0, 0), kTileSize) == 0);
    static_assert(computePixelTileIndex(kWindowSize, float2(kWindowSize), kTileSize) <= kExpectedTileCount);

    static_assert(computeGroupTileIndex(uint3(0, 0, 0), kWindowSize, kTileSize) == 0);
    static_assert(computeGroupTileIndex(uint3(1, 0, 0), kWindowSize, kTileSize) == 1);
    static_assert(computeGroupTileIndex(uint3(0, 1, 0), kWindowSize, kTileSize) == kExpectedGridSize.x);
    static_assert(computeGroupTileIndex(uint3(kExpectedGridSize - 1, 0), kWindowSize, kTileSize) <= kExpectedTileCount);
};

// case 2:
// - window size: (2119, 902)
// - tile size: 16
constexpr void test3() {
    static constexpr uint2 kWindowSize = uint2(2119, 902);
    static constexpr uint kTileSize = 16;

    static constexpr uint2 kExpectedGridSize = uint2(133, 57);
    static constexpr uint kExpectedTileCount = 7581;

    static_assert(computeGridSize(kWindowSize, kTileSize) == kExpectedGridSize);
    static_assert(computeTileCount(kWindowSize, kTileSize) == kExpectedTileCount);

    static_assert(computeWindowTiledSize(kWindowSize, kTileSize) == kExpectedGridSize * kTileSize);

    static_assert(computePixelTileIndex(kWindowSize, float2(0, 0), kTileSize) == 0);
    static_assert(computePixelTileIndex(kWindowSize, float2(kWindowSize), kTileSize) <= kExpectedTileCount);

    static_assert(computeGroupTileIndex(uint3(0, 0, 0), kWindowSize, kTileSize) == 0);
    static_assert(computeGroupTileIndex(uint3(1, 0, 0), kWindowSize, kTileSize) == 1);
    static_assert(computeGroupTileIndex(uint3(0, 1, 0), kWindowSize, kTileSize) == kExpectedGridSize.x);
    static_assert(computeGroupTileIndex(uint3(kExpectedGridSize - 1, 0), kWindowSize, kTileSize) <= kExpectedTileCount);

    static_assert(computeTileCount(kWindowSize, kTileSize) * LIGHT_INDEX_BUFFER_STRIDE == 3896634);
};

constexpr void test4() {
    static constexpr uint2 kWindowSize = uint2(732, 801);
    static constexpr uint kTileSize = 16;
    // static constexpr uint2 kGridSize = uint2(158, 71);
    constexpr uint tileIndexCount = getTileIndexCount(kWindowSize, kTileSize);
    constexpr uint2 gridSize = computeGridSize(kWindowSize, kTileSize);

    static constexpr uint3 SV_GroupId = uint3(gridSize.x - 1, gridSize.y - 1, 0);
    // static constexpr uint3 SV_GroupThreadId = uint3(kTileSize - 1, kTileSize - 1, 0);
    // static constexpr uint3 SV_DispatchThreadId = uint3(
    //     SV_GroupId.x * gridSize.x + SV_GroupThreadId.x,
    //     SV_GroupId.y * gridSize.y + SV_GroupThreadId.y,
    //     0
    // );

    constexpr ViewportData vpd {
        .windowSize = kWindowSize,
    };

    // constexpr uint localIndex = getLocalIndex(SV_GroupThreadId, kTileSize);
    constexpr uint groupIdIndex = vpd.getGroupTileIndex(SV_GroupId, kTileSize);
    constexpr uint startOffset = groupIdIndex * LIGHT_INDEX_BUFFER_STRIDE;

    static_assert((startOffset + LIGHT_INDEX_BUFFER_STRIDE) < tileIndexCount);
}
#endif
