#include "stdafx.hpp"

#include "render/next/components.hpp"

using sm::render::next::CoreDevice;
using sm::render::next::DescriptorPool;

static UINT getHandleIncrement(CoreDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    ID3D12Device1 *it = device.get();
    return it->GetDescriptorHandleIncrementSize(type);
}

static ID3D12DescriptorHeap *newDescriptorHeap(CoreDevice& device, UINT size, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible) noexcept(false) {
    const D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = type,
        .NumDescriptors = size,
        .Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0,
    };

    ID3D12Device1 *it = device.get();
    ID3D12DescriptorHeap *heap;
    SM_THROW_HR(it->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
    return heap;
}

DescriptorPool::DescriptorPool(CoreDevice& device, UINT size, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible) noexcept(false)
    : mDescriptorSize(getHandleIncrement(device, type))
    , mIsShaderVisible(shaderVisible)
    , mAllocator(size)
    , mHeap(newDescriptorHeap(device, size, type, shaderVisible))
{ }

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPool::getHostDescriptorHandle(size_t index) const noexcept {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(getFirstHostHandle(), index, mDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorPool::getDeviceDescriptorHandle(size_t index) const noexcept {
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(getFirstDeviceHandle(), index, mDescriptorSize);
}
