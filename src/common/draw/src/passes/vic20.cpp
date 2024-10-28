#include "draw/passes/vic20.hpp"

using namespace sm::render::next;
using namespace sm::draw::next;

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

enum BindIndex {
    eDrawInfoBuffer = 0, // StructuredBuffer<Vic20Info> : register(t0)
    eFrameBuffer = 1,    // StructuredBuffer<uint>      : register(t0)
    ePresentTexture = 2, // RWTexture2D<float4>         : register(u0)

    eBindCount
};

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

void Vic20Display::createComputeShader() {
    CoreDevice& device = mContext.getCoreDevice();

    /// create root signature
    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    CD3DX12_ROOT_PARAMETER1 params[eBindCount];

    params[eDrawInfoBuffer].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
    params[eFrameBuffer].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);

    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
    params[ePresentTexture].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(params), params, 0, nullptr, kRootFlags);

    mVic20RootSignature = createRootSignature(device, rootSignatureDesc);

    /// create pipeline state
    auto cs = readFileBytes(system::getProgramFolder() / ".." / "vic20.cs.cso");

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = mVic20RootSignature.get(),
        .CS = CD3DX12_SHADER_BYTECODE(cs.data(), cs.size()),
    };

    SM_THROW_HR(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&mVic20PipelineState)));
}

void Vic20Display::createConstBuffers(UINT length) {
    size_t size = sizeof(ConstBufferData) * length;
    mVic20InfoBuffer = BufferResource(mContext, D3D12_RESOURCE_STATE_GENERIC_READ, size, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
}

D3D12_GPU_VIRTUAL_ADDRESS Vic20Display::getInfoBufferAddress(UINT index) const noexcept {
    D3D12_GPU_VIRTUAL_ADDRESS address = mVic20InfoBuffer.deviceAddress();
    return address + index * sizeof(ConstBufferData);
}

void Vic20Display::createFrameBuffer() {
    mVic20FrameBuffer = BufferResource(mContext, D3D12_RESOURCE_STATE_GENERIC_READ, VIC20_FRAMEBUFFER_SIZE, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
}

void Vic20Display::createCompositionTarget(math::uint2 size) {
    // round size up to nearest multiple of 16
    size.x = (size.x + 15) & ~15;
    size.y = (size.y + 15) & ~15;

    DescriptorPool& srvPool = mContext.getSrvHeap();
    CoreDevice& device = mContext.getCoreDevice();
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, size.x, size.y, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
        .Texture2D = {
            .MipSlice = 0,
            .PlaneSlice = 0,
        },
    };

    mTargetSize = size;
    mVic20Target = TextureResource(mContext, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, desc, true);
    mVic20TargetUAVIndex = 1;

    device->CreateUnorderedAccessView(mVic20Target.get(), nullptr, &uavDesc, srvPool.host(mVic20TargetUAVIndex));
}

Vic20Display::Vic20Display(CoreContext& context) noexcept
    : IContextResource(context)
{ }

void Vic20Display::resetDisplayData() {
    mVic20Target.reset();
    mVic20FrameBuffer.reset();
    mVic20InfoBuffer.reset();
}

void Vic20Display::createDisplayData(SurfaceInfo info) {
    createConstBuffers(info.length);
    createFrameBuffer();
    createCompositionTarget(info.size);
}

void Vic20Display::reset() noexcept {
    resetDisplayData();
    mVic20PipelineState.reset();
    mVic20RootSignature.reset();
}

void Vic20Display::create() {
    createComputeShader();
    createDisplayData(mContext.getSwapChainInfo());
}

void Vic20Display::update(SurfaceInfo info) {
    resetDisplayData();
    createDisplayData(info);
}

void Vic20Display::record(ID3D12GraphicsCommandList *list, UINT index) {
    DescriptorPool& srvPool = mContext.getSrvHeap();
    uint dispatchX = mTargetSize.x / 16;
    uint dispatchY = mTargetSize.y / 16;

    ID3D12DescriptorHeap *heaps[] = { srvPool.get() };

    list->SetDescriptorHeaps(_countof(heaps), heaps);

    list->SetComputeRootSignature(mVic20RootSignature.get());
    list->SetPipelineState(mVic20PipelineState.get());

    list->DiscardResource(mVic20Target.get(), nullptr);

    list->SetComputeRootShaderResourceView(eDrawInfoBuffer, getInfoBufferAddress(index));
    list->SetComputeRootShaderResourceView(eFrameBuffer, mVic20FrameBuffer.deviceAddress());
    list->SetComputeRootDescriptorTable(ePresentTexture, srvPool.device(mVic20TargetUAVIndex));

    list->Dispatch(dispatchX, dispatchY, 1);
}
