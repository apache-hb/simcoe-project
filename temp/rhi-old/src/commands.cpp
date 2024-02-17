#include "rhi/rhi.hpp"

using namespace sm;
using namespace sm::rhi;

///
/// command list
///

void CommandList::init(Device& device, Allocator& allocator, CommandListType type) {
    auto dev = device.get_device();
    SM_ASSERT_HR(dev->CreateCommandList(0, type.as_facade(), allocator.get(), nullptr, IID_PPV_ARGS(get_address())));
}

void CommandList::init(Device& device, Allocator& allocator, CommandListType type, PipelineState& pipeline) {
    auto dev = device.get_device();
    SM_ASSERT_HR(dev->CreateCommandList(0, type.as_facade(), allocator.get(), pipeline.get_pipeline(), IID_PPV_ARGS(get_address())));
}

///
/// command allocator
///

void Allocator::init(Device& device, CommandListType type) {
    auto dev = device.get_device();
    SM_ASSERT_HR(dev->CreateCommandAllocator(type.as_facade(), IID_PPV_ARGS(get_address())));
}

///
/// command queue
///

void CommandQueue::init(Device& device, CommandListType type, const char *name) {
    const D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = type.as_facade(),
    };

    auto dev = device.get_device();

    SM_ASSERT_HR(dev->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&m_queue)));

    SM_ASSERT_HR(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

    m_event = CreateEventA(nullptr, FALSE, FALSE, name);
    SM_ASSERT_WIN32(m_event != nullptr);
}

void CommandQueue::execute_commands(ID3D12CommandList *const *commands, UINT count) {
    m_queue->ExecuteCommandLists(count, commands);
}

void CommandQueue::signal(UINT64 value) {
    SM_ASSERT_HR(m_queue->Signal(m_fence.get(), value));
}

void CommandQueue::wait(UINT64 value) {
    if (m_fence->GetCompletedValue() < value) {
        SM_ASSERT_HR(m_fence->SetEventOnCompletion(value, m_event));
        SM_ASSERT_WIN32(WaitForSingleObject(m_event, INFINITE) == WAIT_OBJECT_0);
    }
}
