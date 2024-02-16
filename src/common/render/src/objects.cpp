#include "render/render.hpp"

#include "common.hpp"

#include "d3dx12_core.h"
#include "d3dx12_barriers.h"

using namespace sm;
using namespace sm::render;

void CommandQueue::create(DeviceHandle &device, CommandListType type) {
    const D3D12_COMMAND_QUEUE_DESC desc = {.Type = type.as_facade()};

    SM_ASSERT_HR(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
}

void CommandQueue::signal(Fence &fence, uint64 value) {
    SM_ASSERT_HR(queue->Signal(fence.fence.get(), value));
}

void CommandQueue::execute(uint count, ID3D12CommandList *const *lists) {
    queue->ExecuteCommandLists(count, lists);
}

void DescriptorArena::create(DeviceHandle &device, DescriptorHeapType type, uint count,
                             bool shader_visible) {
    mAllocator.resize(count);
    auto ty = type.as_facade();

    const D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = ty,
        .NumDescriptors = count,
        .Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    SM_ASSERT_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(address())));
    mStride = device->GetDescriptorHandleIncrementSize(ty);
}

DescriptorIndex DescriptorArena::acquire() {
    auto index = mAllocator.scan_set_first();
    CTASSERTF(index != sm::BitMap::eInvalid, "Out of space in bitmap arena");
    return DescriptorIndex(index);
}

void DescriptorArena::release(DescriptorIndex handle) {
    mAllocator.release(sm::BitMap::Index(handle));
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorArena::cpu(DescriptorIndex index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = get()->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<uint64>(mStride * index);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorArena::gpu(DescriptorIndex index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = get()->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<uint64>(mStride * index);
    return handle;
}

void CommandListPool::create(DeviceHandle &device) {
    for (size_t i = 0; i < ResourcePool::length(); i++) {
        auto &list = ResourcePool::mStorage[i];
        list.create(device, mType);
    }
}

CommandList *CommandListPool::acquire() {
    return ResourcePool::acquire();
}

void Fence::create(DeviceHandle &device, const char *name) {
    SM_ASSERT_HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    event = CreateEvent(nullptr, FALSE, FALSE, name);
    SM_ASSERT_HR(event.is_valid());
}

void Fence::wait(uint64 pending) {
    if (value() < pending) {
        SM_ASSERT_HR(fence->SetEventOnCompletion(pending, *event));
        WaitForSingleObject(*event, INFINITE);
    }
}

void CommandList::create(DeviceHandle &device, CommandListType type) {
    SM_ASSERT_HR(device->CreateCommandAllocator(type.as_facade(), IID_PPV_ARGS(&allocator)));
    SM_ASSERT_HR(device->CreateCommandList(0, type.as_facade(), allocator.get(), nullptr,
                                           IID_PPV_ARGS(&list)));
}

void CommandList::close() {
    SM_ASSERT_HR(list->Close());
}

void CommandList::reset() {
    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(list->Reset(allocator.get(), nullptr));
}

void CommandList::copy_buffer(DeviceResource &src, DeviceResource &dst, uint size) {
    list->CopyBufferRegion(dst.resource.get(), 0, src.resource.get(), 0, size);
}

void PendingObject::dispose(Context &ctx) const {
    mDispose(mObject, ctx);
}

void DeviceResource::write(const void *data, size_t size) {
    void *ptr = nullptr;
    D3D12_RANGE range{};
    SM_ASSERT_HR(resource->Map(0, &range, &ptr));
    memcpy(ptr, data, size);
    resource->Unmap(0, nullptr);
}

static D3D12_RESOURCE_STATES as_facade(ResourceState state) {
    switch (state) {
    case ResourceState::eConstBuffer:
    case ResourceState::eVertexBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case ResourceState::eIndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;

    case ResourceState::eCopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case ResourceState::eCopyTarget: return D3D12_RESOURCE_STATE_COPY_DEST;

    default: NEVER("unsupported resource state");
    }
}

Barrier render::transition_barrier(DeviceResource &resource, ResourceState before, ResourceState after) {
    auto before_state = as_facade(before);
    auto after_state = as_facade(after);

    return CD3DX12_RESOURCE_BARRIER::Transition(resource.get(), before_state, after_state);
}

VertexBufferView render::vbo_view(DeviceResource &resource, uint stride, uint size) {
    const VertexBufferView view = {
        .address = resource.gpu_address(),
        .size = size,
        .stride = stride,
    };

    return view;
}

IndexBufferView render::ibo_view(DeviceResource &resource, uint size, DataFormat type) {
    const IndexBufferView view = {
        .address = resource.gpu_address(),
        .size = size,
        .format = type,
    };

    return view;
}

static D3D12_SHADER_BYTECODE as_bytecode(const ShaderBytecode& bc) {
    return { (const void*)bc.data.data(), bc.data.size_bytes() };
}

void PipelineState::create(DeviceHandle &device, const PipelineConfig &config) {
    sm::SmallVector<D3D12_INPUT_ELEMENT_DESC, 4> layout;
    for (auto elem : config.input) {
        D3D12_INPUT_ELEMENT_DESC desc = {
            .SemanticName = elem.name,
            .Format = elem.format.as_facade(),
            .AlignedByteOffset = elem.offset,
            .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        };
        layout.push_back(desc);
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
        //.pRootSignature = config.root_signature.get(),
        .VS = as_bytecode(config.vs),
        .PS = as_bytecode(config.ps),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        .InputLayout = { layout.data(), uint(layout.size()) },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = { 1, 0 },
    };

    SM_ASSERT_HR(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mState)));
}
