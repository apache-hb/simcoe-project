#include "render/object.hpp"
#include "render/render.hpp"

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
    return mHandle->Map(0, range, data);
}

void Resource::unmap(const D3D12_RANGE *range) {
    mHandle->Unmap(0, range);
}

D3D12_GPU_VIRTUAL_ADDRESS Resource::get_gpu_address() {
    return mHandle->GetGPUVirtualAddress();
}

void Resource::reset() {
    mHandle.reset();
    mAllocation.reset();
}

void IDependency::create(Context& context, DependsOn reason) {
    do_create(context, reason);
}

void IDependency::destroy(Context& context, DependsOn reason) {
    do_destroy(context, reason);
}

void IDependency::depend(DependsOn dep) {
    depends |= dep;
}
