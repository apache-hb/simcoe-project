#include "render/object.hpp"
#include "render/render.hpp"

#include "d3dx12/d3dx12_root_signature.h"

using namespace sm;
using namespace sm::render;

std::string_view Blob::as_string() const {
    return {reinterpret_cast<const char*>(data()), size()};
}

const void *Blob::data() const {
    return get()->GetBufferPointer();
}

size_t Blob::size() const {
    return get()->GetBufferSize();
}

Result Resource::map(const D3D12_RANGE *range, void **data) {
    return mResource->Map(0, range, data);
}

void Resource::unmap(const D3D12_RANGE *range) {
    mResource->Unmap(0, range);
}

D3D12_GPU_VIRTUAL_ADDRESS Resource::get_gpu_address() {
    return mResource->GetGPUVirtualAddress();
}

void Resource::reset() {
    mResource.reset();
    mAllocation.reset();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::cpu_descriptor_handle(int index) {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(get()->GetCPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::gpu_descriptor_handle(int index) {
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(get()->GetGPUDescriptorHandleForHeapStart(), index, mDescriptorSize);
}

Viewport::Viewport(math::uint2 size) {
    auto [w, h] = size;

    mViewport = {
        .TopLeftX = 0.f,
        .TopLeftY = 0.f,
        .Width = float(w),
        .Height = float(h),
        .MinDepth = 0.f,
        .MaxDepth = 1.f,
    };

    mScissorRect = {
        .left = 0,
        .top = 0,
        .right = int_cast<LONG>(w),
        .bottom = int_cast<LONG>(h),
    };
}

void Pipeline::reset() {
    mRootSignature.reset();
    mPipelineState.reset();
}

void DescriptorAllocator::set_heap(DescriptorHeap heap) {
    mHeap = std::move(heap);
}

DescriptorIndex DescriptorAllocator::allocate() {
    auto index = mAllocator.allocate();
    SM_ASSERTF(index != sm::BitMap::eInvalid, "DescriptorAllocator: allocate out of descriptors. size {}", mHeap.mCapacity);
    auto ret = DescriptorIndex(index);
    SM_ASSERTF(ret < mHeap.mCapacity, "DescriptorAllocator: allocate out of range. index {} capacity {}", ret, mHeap.mCapacity);
    return ret;
}

void DescriptorAllocator::release(DescriptorIndex index) {
    SM_ASSERTF(index < mHeap.mCapacity, "DescriptorAllocator: release out of range. index {} capacity {}", index, mHeap.mCapacity);
    mAllocator.release(sm::BitMap::Index{index});
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::cpu_descriptor_handle(int index) {
    return mHeap.cpu_descriptor_handle(index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::gpu_descriptor_handle(int index) {
    return mHeap.gpu_descriptor_handle(index);
}
