#include "draw/passes/vic20.hpp"

#include <directx/d3dx12_barriers.h>

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
    mInfoBuffer = InfoDeviceBuffer(mContext, length, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);

    for (size_t i = 0; i < length; i++) {
        shared::Vic20Info data = {
            .textureSize = mTargetSize,
            .dispatchSize = (mTargetSize / math::uint2(VIC20_THREADS_X, VIC20_THREADS_Y)),
        };

        mInfoBuffer.updateElement(i, data);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS Vic20Display::getInfoBufferAddress(UINT index) const noexcept {
    return mInfoBuffer.elementAddress(index);
}

void Vic20Display::createFrameBuffer() {
    size_t size = size_t(VIC20_SCREEN_SIZE);
    mFrameBuffer = DeviceFrameBuffer(mContext, size, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
    mFrameBufferPtr = mFrameBuffer.map(size);
    mFrameData = std::make_unique<FrameBufferElement[]>(size);
}

void Vic20Display::createCompositionTarget() {
    DescriptorPool& srvPool = mContext.getSrvHeap();
    CoreDevice& device = mContext.getCoreDevice();
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM, mTargetSize.x, mTargetSize.y,
        1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
        .Texture2D = {
            .MipSlice = 0,
            .PlaneSlice = 0,
        },
    };

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = 1,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f,
        },
    };

    mTarget = TextureResource(mContext, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, desc, true);
    mTargetUAVIndex = srvPool.allocate();
    mTargetSRVIndex = srvPool.allocate();

    device->CreateUnorderedAccessView(mTarget.get(), nullptr, &uavDesc, srvPool.host(mTargetUAVIndex));
    device->CreateShaderResourceView(mTarget.get(), &srvDesc, srvPool.host(mTargetSRVIndex));
}

Vic20Display::Vic20Display(CoreContext& context, math::uint2 resolution) noexcept
    : IContextResource(context)
    , mTargetSize(resolution)
{
    mTargetSize.x = (mTargetSize.x + VIC20_THREADS_X) & ~VIC20_THREADS_X;
    mTargetSize.y = (mTargetSize.y + VIC20_THREADS_Y) & ~VIC20_THREADS_Y;
}

void Vic20Display::resetDisplayData() {
    DescriptorPool& srvPool = mContext.getSrvHeap();
    srvPool.free(mTargetUAVIndex);
    srvPool.free(mTargetSRVIndex);

    mTarget.reset();
    mFrameBuffer.reset();
    mInfoBuffer.reset();
}

void Vic20Display::createDisplayData(SurfaceInfo info) {
    createConstBuffers(info.length);
    createFrameBuffer();
    createCompositionTarget();
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
    ID3D12DescriptorHeap *heaps[] = { srvPool.get() };
    uint dispatchX = (mTargetSize.x / VIC20_THREADS_X) - 1;
    uint dispatchY = (mTargetSize.y / VIC20_THREADS_Y) - 1;

    std::copy(mFrameData.get(), mFrameData.get() + VIC20_SCREEN_SIZE, mFrameBufferPtr);

    const D3D12_RESOURCE_BARRIER cBarrier[] = {
        CD3DX12_RESOURCE_BARRIER::UAV(getTarget())
    };

    list->SetDescriptorHeaps(_countof(heaps), heaps);

    list->SetComputeRootSignature(mVic20RootSignature.get());
    list->SetPipelineState(mVic20PipelineState.get());

    list->DiscardResource(getTarget(), nullptr);

    list->SetComputeRootShaderResourceView(eDrawInfoBuffer, getInfoBufferAddress(index));
    list->SetComputeRootShaderResourceView(eFrameBuffer, mFrameBuffer.deviceAddress());
    list->SetComputeRootDescriptorTable(ePresentTexture, srvPool.device(mTargetUAVIndex));

    list->ResourceBarrier(_countof(cBarrier), cBarrier);

    list->Dispatch(dispatchX, dispatchY, 1);
}

D3D12_GPU_DESCRIPTOR_HANDLE Vic20Display::getTargetSrv() const {
    DescriptorPool& srvPool = mContext.getSrvHeap();
    return srvPool.device(mTargetSRVIndex);
}

void Vic20Display::write(uint8_t x, uint8_t y, uint8_t colour) noexcept {
    if (x >= VIC20_SCREEN_WIDTH || y >= VIC20_SCREEN_HEIGHT) {
        return;
    }

    uint32_t coord = uint32_t(y) * VIC20_SCREEN_WIDTH + x;
    mFrameData[coord] = colour;
#if 0
    int high = (coord & 1);
    int index = (coord & ~1) / 2;
    uint8_t palette = colour & 0x0F;

    uint8_t& cell = mFrameData[index];
    if (high) {
        cell = (cell & 0b1111'0000) | palette;
    } else {
        cell = (cell & 0b0000'1111) | (palette << 4);
    }
#endif
}
