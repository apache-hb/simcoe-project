#include "stdafx.hpp"

#include "render/next/context/resource.hpp"

using namespace sm::render;
using namespace sm::render::next;

static D3D12MA::ALLOCATION_DESC newAllocInfo(bool comitted, D3D12_HEAP_TYPE heap) {
    return D3D12MA::ALLOCATION_DESC {
        .Flags = comitted ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_NONE,
        .HeapType = heap,
    };
}

DeviceResource::DeviceResource(CoreContext& context, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc, D3D12MA::ALLOCATION_DESC alloc) {
    D3D12MA::Allocator *allocator = context.getAllocator();

    SM_THROW_HR(allocator->CreateResource(&alloc, &desc, state, nullptr, &mResource, __uuidof(ID3D12Resource), nullptr));
}

TextureResource::TextureResource(CoreContext& context, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc, bool comitted)
    : DeviceResource(context, state, desc, newAllocInfo(comitted, D3D12_HEAP_TYPE_DEFAULT))
{ }

BufferResource::BufferResource(CoreContext& context, D3D12_RESOURCE_STATES state, uint64 width, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heap)
    : DeviceResource(context, state, CD3DX12_RESOURCE_DESC::Buffer(width, flags), newAllocInfo(false, heap))
{ }

void BufferResource::write(const void *data, size_t size) {
    D3D12_RANGE written = {0, size};
    void *mapped = map(size);
    memcpy(mapped, data, size);
    unmap(mapped, &written);
}

void *BufferResource::map(size_t size) {
    ID3D12Resource *resource = get();

    void *mapped;
    SM_THROW_HR(resource->Map(0, nullptr, &mapped));

    return mapped;
}

void BufferResource::unmap(void *mapped, const D3D12_RANGE *written) noexcept {
    ID3D12Resource *resource = get();
    resource->Unmap(0, written);
}

static ConstBufferData newConstBuffer(CoreContext& context, UINT index, size_t size) {
    Object<D3D12MA::Allocation> resource;
    D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle;
    void *data = nullptr;

    return ConstBufferData {std::move(resource), cbvHandle, data};
}

ConstBufferResourceBase::ConstBufferResourceBase(CoreContext& context, size_t size)
    : Super(context.getSwapChainLength(), [&](UINT index) { return newConstBuffer(context, index, size); })
{ }

void ConstBufferResourceBase::update(UINT frame, const void *data, size_t size) {
    ConstBufferData& buffer = get(frame);
    memcpy(buffer.data, data, size);
}
