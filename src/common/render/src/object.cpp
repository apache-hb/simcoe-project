#include "render/object.hpp"
#include "render/render.hpp"

#include "stdafx.hpp"

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

Result Resource::write(const void *data, size_t size) {
    D3D12_RANGE read{0, 0};
    void *dst;
    auto result = map(&read, &dst);
    if (result.failed()) return result;

    std::memcpy(dst, data, size);
    unmap(&read);

    return HRESULT_FROM_WIN32(ERROR_SUCCESS);
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
