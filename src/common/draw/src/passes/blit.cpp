#include "draw/passes/blit.hpp"

#include "system/storage.hpp"

#include <directx/d3dx12_barriers.h>

using namespace sm;
using namespace sm::render::next;
using namespace sm::draw::next;

struct Vertex {
    math::float2 position;
    math::float2 uv;
};

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

static std::vector<uint8_t> readFileBytes(const sm::fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + path.string());
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();

    std::vector<uint8_t> data(size);
    file.seekg(0, std::ios::beg);

    file.read(reinterpret_cast<char *>(data.data()), size);

    return data;
}

BlitPass::BlitPass(render::next::CoreContext& context, math::uint2 resolution, DXGI_FORMAT format)
    : Super(context)
    , mTargetSize(resolution)
    , mTargetFormat(format)
{ }

void BlitPass::record(ID3D12GraphicsCommandList *list, UINT index, D3D12_GPU_DESCRIPTOR_HANDLE source) {
    list->SetPipelineState(mPipelineState.get());
    list->SetGraphicsRootSignature(mRootSignature.get());

    list->OMSetRenderTargets(1, &mBlitRtv, FALSE, nullptr);

    D3D12_VIEWPORT viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(mTargetSize.x),
        .Height = static_cast<float>(mTargetSize.y),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };

    D3D12_RECT scissor = {
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(mTargetSize.x),
        .bottom = static_cast<LONG>(mTargetSize.y),
    };

    list->RSSetViewports(1, &viewport);
    list->RSSetScissorRects(1, &scissor);

    list->SetGraphicsRootDescriptorTable(0, source);

    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    list->IASetVertexBuffers(0, 1, &mVertexBufferView);

    list->DrawInstanced(4, 1, 0, 0);
}

D3D12_GPU_DESCRIPTOR_HANDLE BlitPass::getTargetSrv() const {
    return mBlitSrv;
}

void BlitPass::createShader() {
    CoreDevice& device = mContext.getCoreDevice();

    // create shader

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    CD3DX12_ROOT_PARAMETER1 params[1];

    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
    params[0].InitAsDescriptorTable(1, ranges, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];

    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(params), params, _countof(samplers), samplers, kRootFlags);

    mRootSignature = createRootSignature(device, rootSignatureDesc);

    auto ps = readFileBytes(system::getProgramDataFolder() / "blit.ps.cso");
    auto vs = readFileBytes(system::getProgramDataFolder() / "blit.vs.cso");

    constexpr D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, position) },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv) },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = mRootSignature.get(),
        .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
        .PS = CD3DX12_SHADER_BYTECODE(ps.data(), ps.size()),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .InputLayout = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { mTargetFormat },
        .SampleDesc = { 1, 0 },
    };

    SM_THROW_HR(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState)));
}

void BlitPass::createTarget(math::uint2 size, DXGI_FORMAT format) {
    CoreDevice& device = mContext.getCoreDevice();
    DescriptorPool& srvPool = mContext.getSrvHeap();
    DescriptorPool& rtvPool = mContext.getRtvHeap();

    mBlitTarget = TextureResource(
        mContext,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        CD3DX12_RESOURCE_DESC::Tex2D(format, size.x, size.y, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
        true
    );

    size_t srvIndex = srvPool.allocate();
    size_t rtvIndex = rtvPool.allocate();

    mBlitRtv = rtvPool.host(rtvIndex);
    mBlitSrv = srvPool.device(srvIndex);

    device->CreateRenderTargetView(mBlitTarget.get(), nullptr, rtvPool.host(rtvIndex));
    device->CreateShaderResourceView(mBlitTarget.get(), nullptr, srvPool.host(srvIndex));

    mBlitTarget.get()->SetName(L"Blit Target");
}

void BlitPass::createQuad() {
    CoreDevice& device = mContext.getCoreDevice();

    Vertex vertices[] = {
        { { -1.0f, -1.0f }, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f }, { 0.0f, 0.0f } },
        { {  1.0f, -1.0f }, { 1.0f, 1.0f } },
        { {  1.0f,  1.0f }, { 1.0f, 0.0f } },
    };

    mVertexBuffer = AnyBufferResource(
        mContext,
        sizeof(vertices),
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_HEAP_TYPE_UPLOAD
    );

    mVertexBuffer.write(vertices, sizeof(vertices));

    mVertexBufferView = D3D12_VERTEX_BUFFER_VIEW {
        .BufferLocation = mVertexBuffer.deviceAddress(),
        .SizeInBytes = sizeof(vertices),
        .StrideInBytes = sizeof(Vertex),
    };

    mVertexBuffer.get()->SetName(L"Blit Vertex Buffer");
}

void BlitPass::setup() {
    createShader();
    createTarget(mTargetSize, mTargetFormat);
    createQuad();
}

void BlitPass::reset() noexcept {
    mVertexBuffer.reset();
    mRootSignature.reset();
    mPipelineState.reset();
    mBlitTarget.reset();
    mContext.getSrvHeap().free(mBlitSrv);
    mContext.getRtvHeap().free(mBlitRtv);
}

void BlitPass::create() {
    setup();
}
