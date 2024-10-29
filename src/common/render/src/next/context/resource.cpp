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
    : Super(context, state, desc, newAllocInfo(comitted, D3D12_HEAP_TYPE_DEFAULT))
{ }

///
/// unbuffered resource
///

AnyBufferResource::AnyBufferResource(CoreContext& context, uint64_t width, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heap)
    : Super(context, state, CD3DX12_RESOURCE_DESC::Buffer(width, flags), newAllocInfo(false, heap))
{ }

void AnyBufferResource::write(const void *data, size_t size) {
    D3D12_RANGE written = {0, size};
    void *mapped = map(size);
    memcpy(mapped, data, size);
    unmap(mapped, &written);
}

void *AnyBufferResource::map(size_t size) {
    ID3D12Resource *resource = get();

    void *mapped;
    SM_THROW_HR(resource->Map(0, nullptr, &mapped));

    return mapped;
}

void AnyBufferResource::unmap(void *mapped, const D3D12_RANGE *written) noexcept {
    ID3D12Resource *resource = get();
    resource->Unmap(0, written);
}

///
/// buffered per frame resource
///

AnyMultiBufferResource::AnyMultiBufferResource(CoreContext& context, size_t count, size_t size, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heap)
    : Super(context, (count * size), state, flags, heap)
    , mElementCount(count)
    , mMapped(map(count * size))
{ }

void *AnyMultiBufferResource::getElementData(size_t index, size_t stride) const {
    CTASSERTF(index < mElementCount, "Index out of bounds: %zu >= %u", index, mElementCount);
    return static_cast<uint8_t *>(mMapped) + (index * stride);
}

D3D12_GPU_VIRTUAL_ADDRESS AnyMultiBufferResource::getElementDeviceAddress(size_t index, size_t stride) const {
    CTASSERTF(index < mElementCount, "Index out of bounds: %zu >= %u", index, mElementCount);
    return deviceAddress() + (index * stride);
}

void AnyMultiBufferResource::update(UINT frame, const void *data, size_t stride) {
    void *element = getElementData(frame, stride);
    memcpy(element, data, stride);
}
