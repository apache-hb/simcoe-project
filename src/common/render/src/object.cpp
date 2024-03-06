#include "render/object.hpp"
#include "render/render.hpp"
#include "render/commands.hpp"

using namespace sm;
using namespace sm::render;

void CommandList::reset(ID3D12CommandAllocator *allocator, ID3D12PipelineState *pso) {
    SM_ASSERT_HR(get()->Reset(allocator, pso));
}

void CommandList::submit_barriers(sm::Span<const D3D12_RESOURCE_BARRIER> barriers) {
    if (barriers.empty()) return;

    get()->ResourceBarrier(barriers.size(), barriers.data());
}

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

void Resource::rename(sm::StringView name) {
    if (mResource) mResource.rename(name);
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
    signature.reset();
    pso.reset();
}
