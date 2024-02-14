#include "render/render.hpp"

#include "common.hpp"

using namespace sm::render;

void CommandQueue::create(DeviceHandle& device, CommandListType type) {
    const D3D12_COMMAND_QUEUE_DESC desc = {
        .Type = type.as_facade()
    };

    SM_ASSERT_HR(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
}

void CommandQueue::signal(Fence& fence, UINT64 value) {
    SM_ASSERT_HR(queue->Signal(fence.fence.get(), value));
}

void CommandQueue::execute(UINT count, ID3D12CommandList *const *lists) {
    queue->ExecuteCommandLists(count, lists);
}

void DescriptorArena::create(DeviceHandle& device, DescriptorHeapType type, UINT count, bool shader_visible) {
    mAllocator.resize(count);
    auto ty = type.as_facade();

    const D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = ty,
        .NumDescriptors = count,
        .Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    SM_ASSERT_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mDescriptorHeap)));
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
    D3D12_CPU_DESCRIPTOR_HANDLE handle = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(mStride * index);
    return handle;

}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorArena::gpu(DescriptorIndex index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(mStride * index);
    return handle;
}

void Fence::create(DeviceHandle& device, const char *name) {
    SM_ASSERT_HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    event = CreateEvent(nullptr, FALSE, FALSE, name);
    SM_ASSERT_HR(event.is_valid());
}

void Fence::wait(UINT64 value) {
    if (fence->GetCompletedValue() < value) {
        SM_ASSERT_HR(fence->SetEventOnCompletion(value, *event));
        WaitForSingleObject(*event, INFINITE);
    }
}

void CommandList::create(DeviceHandle& device, CommandListType type) {
    SM_ASSERT_HR(device->CreateCommandAllocator(type.as_facade(), IID_PPV_ARGS(&allocator)));
    SM_ASSERT_HR(device->CreateCommandList(0, type.as_facade(), allocator.get(), nullptr, IID_PPV_ARGS(&list)));
}

void CommandList::close() {
    SM_ASSERT_HR(list->Close());
}

void CommandList::reset() {
    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(list->Reset(allocator.get(), nullptr));
}
