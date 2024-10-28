#include "render/next/components.hpp"

using namespace sm::render;

void next::syncDeviceTimeline(ID3D12CommandQueue *dependent, ID3D12CommandQueue *independent, ID3D12Fence *fence, uint64_t value) {
    SM_THROW_HR(dependent->Signal(fence, value));
    SM_THROW_HR(independent->Wait(fence, value));
}
