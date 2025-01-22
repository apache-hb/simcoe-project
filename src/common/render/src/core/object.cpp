#include "stdafx.hpp"

#include "core/units.hpp"

#include "render/base/object.hpp"
#include "render/base/resource.hpp"

using namespace sm;
using namespace sm::render;

std::string_view Blob::asString() const {
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

void Pipeline::rename(std::string_view name) {
    if (signature) signature.rename(fmt::format("{}.signature", name));
    if (pso) pso.rename(fmt::format("{}.pso", name));
}

void render::setObjectDebugName(ID3D12Object *object, std::string_view name) {
    auto wide = sm::widen(name);
    object->SetName(wide.c_str());
}

std::string render::getObjectDebugName(ID3D12Object *object) {
    UINT size = 0;
    object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, nullptr);

    if (size == 0) return {};

    std::wstring wide;
    wide.resize(size / sizeof(wchar_t) + 1);

    object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, wide.data());

    wide.resize(size / sizeof(wchar_t));

    return sm::narrow(wide);
}
