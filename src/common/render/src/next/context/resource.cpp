#include "stdafx.hpp"

#include "render/next/context/resource.hpp"

using namespace sm::render;
using namespace sm::render::next;

static ConstBufferData newConstBuffer(UINT index, size_t size) {
    Object<D3D12MA::Allocation> resource;
    D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle;
    void *data = nullptr;

    return ConstBufferData {std::move(resource), cbvHandle, data};
}

ConstBufferResourceBase::ConstBufferResourceBase(UINT frames, size_t size)
    : Super(frames, [&](UINT index) { return newConstBuffer(index, size); })
{ }

void ConstBufferResourceBase::update(UINT frame, const void *data, size_t size) {
    ConstBufferData& buffer = get(frame);
    memcpy(buffer.data, data, size);
}
