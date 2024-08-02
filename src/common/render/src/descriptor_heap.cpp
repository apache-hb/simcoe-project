#include "stdafx.hpp"

#include "render/descriptor_heap.hpp"

using namespace sm;
using namespace sm::render;

HRESULT DescriptorPoolBase::createDescriptorHeap(ID3D12Device1 *device, const D3D12_DESCRIPTOR_HEAP_DESC &desc) noexcept {
    mDescriptorSize = device->GetDescriptorHandleIncrementSize(desc.Type);
    mAllocator.resize(desc.NumDescriptors);

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap));
    if (FAILED(hr))
        return hr;

    mIsShaderVisible = (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    mFirstHostHandle = mHeap->GetCPUDescriptorHandleForHeapStart();

    if (isShaderVisible())
        mFirstDeviceHandle = mHeap->GetGPUDescriptorHandleForHeapStart();

    return hr;
}

void DescriptorPoolBase::release(size_t index) noexcept {
    CTASSERTF(mAllocator.test(index), "double free. Index %zu is already free", index);
    mAllocator.deallocate(index);
}

bool DescriptorPoolBase::tryRelease(size_t index) noexcept {
    bool ok = (index != kInvalidIndex) && mAllocator.test(index);

    if (ok)
        mAllocator.deallocate(index);

    return ok;
}

HostDescriptorHandle DescriptorPoolBase::getHostDescriptorHandle(size_t index) const noexcept {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(getBaseHostHandle(), index, mDescriptorSize);
}

DeviceDescriptorHandle DescriptorPoolBase::getDeviceDescriptorHandle(size_t index) const noexcept {
    CTASSERT(isShaderVisible());
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(getBaseDeviceHandle(), index, mDescriptorSize);
}
