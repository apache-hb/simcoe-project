#include "stdafx.hpp"

#include "render/next/components.hpp"

using sm::render::next::CoreDevice;
using sm::render::next::DescriptorPool;

static UINT getHandleIncrement(CoreDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    return device->GetDescriptorHandleIncrementSize(type);
}

static ID3D12DescriptorHeap *newDescriptorHeap(CoreDevice& device, UINT size, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible) noexcept(false) {
    const D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = type,
        .NumDescriptors = size,
        .Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0,
    };

    ID3D12DescriptorHeap *heap;
    SM_THROW_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
    return heap;
}

DescriptorPool::DescriptorPool(CoreDevice& device, UINT size, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible) noexcept(false)
    : mDescriptorSize(getHandleIncrement(device, type))
    , mIsShaderVisible(shaderVisible)
    , mAllocator(size)
    , mHeap(newDescriptorHeap(device, size, type, shaderVisible))
    , mFirstHostHandle(mHeap->GetCPUDescriptorHandleForHeapStart())
    , mFirstDeviceHandle(isShaderVisible() ? mHeap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{})
{ }

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPool::host(size_t index) const noexcept {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(getFirstHostHandle(), index, mDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorPool::device(size_t index) const noexcept {
    CTASSERT(isShaderVisible());
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(getFirstDeviceHandle(), index, mDescriptorSize);
}

size_t DescriptorPool::allocate() {
    size_t index = mAllocator.allocateIndex();
    if (index == BitMapIndexAllocator::kInvalidIndex) {
        throw RenderException(HRESULT_FROM_WIN32(ERROR_TOO_MANY_DESCRIPTORS), "Failed to allocate descriptor");
    }

    return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorPool::allocateHost() {
    return host(allocate());
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorPool::allocateDevice() {
    return device(allocate());
}

void DescriptorPool::free(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
    UINT64 offset = (handle.ptr - mFirstDeviceHandle.ptr) / mDescriptorSize;
    mAllocator.deallocate(offset);
}

void DescriptorPool::free(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    UINT64 offset = (handle.ptr - mFirstHostHandle.ptr) / mDescriptorSize;
    mAllocator.deallocate(offset);
}
