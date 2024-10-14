#include "stdafx.hpp"

#include "render/base/resource.hpp"
#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

#if 0
void BufferResource::reset() noexcept {
    mAllocation.reset();
    mDeviceAddress = 0;
}

void BufferResource::rename(std::string_view name) noexcept {
    ID3D12Resource *resource = getResource();
    std::wstring wide = sm::widen(name);

    mAllocation->SetName(wide.c_str());
    resource->SetName(wide.c_str());
}

void* BufferResource::mapWriteOnly() noexcept {
    ID3D12Resource *resource = getResource();

    void *data = nullptr;
    SM_ASSERT_HR(resource->Map(0, nullptr, &data));

    return data;
}

void BufferResource::unmap(D3D12_RANGE written) noexcept {
    ID3D12Resource *resource = getResource();
    resource->Unmap(0, &written);
}

void BufferResource::unmap() noexcept {
    ID3D12Resource *resource = getResource();
    resource->Unmap(0, nullptr);
}

BufferResource::BufferResource(
    IDeviceContext& context,
    const D3D12_RESOURCE_DESC& desc,
    const D3D12MA::ALLOCATION_DESC& alloc,
    D3D12_RESOURCE_STATES state
) noexcept {
    D3D12MA::Allocator *allocator = context.getAllocator();
    SM_ASSERT_HR(allocator->CreateResource(&alloc, &desc, state, nullptr, &mAllocation,  __uuidof(ID3D12Resource), nullptr));

    ID3D12Resource *resource = getResource();
    mDeviceAddress = resource->GetGPUVirtualAddress();
}
#endif
