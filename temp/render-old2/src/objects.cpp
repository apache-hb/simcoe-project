#include "render/render.hpp"

#include "common.hpp"

#include "d3dx12_barriers.h"
#include "d3dx12_core.h"
#include "d3dx12_root_signature.h"

using namespace sm;
using namespace sm::render;

void CommandQueue::create(DeviceHandle &device, CommandListType type) {
    const D3D12_COMMAND_QUEUE_DESC desc = {.Type = type.as_facade()};

    SM_ASSERT_HR(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
}

void CommandQueue::signal(Fence &fence, uint64 value) {
    fmt::println("signaling fence: {}", value);
    SM_ASSERT_HR(queue->Signal(fence.fence.get(), value));
}

void CommandQueue::execute(uint count, ID3D12CommandList *const *lists) {
    queue->ExecuteCommandLists(count, lists);
}

void DescriptorArena::create(DeviceHandle &device, DescriptorHeapType type, uint count,
                             bool shader_visible) {
    mAllocator.resize(count);
    auto ty = type.as_facade();

    const D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type = ty,
        .NumDescriptors = count,
        .Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    SM_ASSERT_HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(address())));
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
    D3D12_CPU_DESCRIPTOR_HANDLE handle = get()->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<uint64>(mStride * index);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorArena::gpu(DescriptorIndex index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = get()->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<uint64>(mStride * index);
    return handle;
}

void CommandListPool::create(DeviceHandle &device) {
    for (size_t i = 0; i < ResourcePool::length(); i++) {
        auto &list = ResourcePool::mStorage[i];
        list.create(device, mType);
    }
}

CommandList *CommandListPool::acquire(ResourceId id) {
    return ResourcePool::acquire(id);
}

void Fence::create(DeviceHandle &device, const char *name) {
    SM_ASSERT_HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    event = CreateEvent(nullptr, FALSE, FALSE, name);
    SM_ASSERT_HR(event.is_valid());
}

void Fence::wait(uint64 pending) {
    auto current = value();
    fmt::println("waiting for fence: {} < {}", current, pending);
    if (current < pending) {
        SM_ASSERT_HR(fence->SetEventOnCompletion(pending, *event));
        WaitForSingleObjectEx(*event, INFINITE, false);
    }
}

uint64 Fence::value() {
    return fence->GetCompletedValue();
}

void CommandList::create(DeviceHandle &device, CommandListType type) {
    SM_ASSERT_HR(device->CreateCommandAllocator(type.as_facade(), IID_PPV_ARGS(&allocator)));
    SM_ASSERT_HR(device->CreateCommandList(0, type.as_facade(), allocator.get(), nullptr,
                                           IID_PPV_ARGS(&list)));
}

void CommandList::close() {
    SM_ASSERT_HR(list->Close());
}

void CommandList::reset() {
    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(list->Reset(allocator.get(), nullptr));
}

void CommandList::set_index_buffer(const IndexBufferView &view) {
    const D3D12_INDEX_BUFFER_VIEW desc = {
        .BufferLocation = view.address.as_integral(),
        .SizeInBytes = view.size,
        .Format = view.format.as_facade(),
    };

    list->IASetIndexBuffer(&desc);
}

void CommandList::set_vertex_buffer(const VertexBufferView &view) {
    const D3D12_VERTEX_BUFFER_VIEW desc = {
        .BufferLocation = view.address.as_integral(),
        .SizeInBytes = view.size,
        .StrideInBytes = view.stride,
    };

    list->IASetVertexBuffers(0, 1, &desc);
}


void CommandList::copy_buffer(DeviceResource &src, DeviceResource &dst, uint size) {
    list->CopyBufferRegion(dst.resource.get(), 0, src.resource.get(), 0, size);
}

void DeviceResource::write(const void *data, size_t size) {
    void *ptr = nullptr;
    D3D12_RANGE range{};
    SM_ASSERT_HR(resource->Map(0, &range, &ptr));
    memcpy(ptr, data, size);
    resource->Unmap(0, nullptr);
}

static D3D12_RESOURCE_STATES as_facade(ResourceState state) {
    switch (state) {
    case ResourceState::eConstBuffer:
    case ResourceState::eVertexBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case ResourceState::eIndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;

    case ResourceState::eCopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case ResourceState::eCopyTarget: return D3D12_RESOURCE_STATE_COPY_DEST;

    default: NEVER("unsupported resource state");
    }
}

Barrier render::transition_barrier(DeviceResource &resource, ResourceState before,
                                   ResourceState after) {
    auto before_state = as_facade(before);
    auto after_state = as_facade(after);

    return CD3DX12_RESOURCE_BARRIER::Transition(resource.get(), before_state, after_state);
}

VertexBufferView render::vbo_view(DeviceResource &resource, uint stride, uint size) {
    const VertexBufferView view = {
        .address = resource.gpu_address(),
        .size = size,
        .stride = stride,
    };

    return view;
}

IndexBufferView render::ibo_view(DeviceResource &resource, uint size, DataFormat type) {
    const IndexBufferView view = {
        .address = resource.gpu_address(),
        .size = size,
        .format = type,
    };

    return view;
}

std::string_view Blob::as_string() const {
    return {(const char *)get()->GetBufferPointer(), get()->GetBufferSize()};
}

const void *Blob::data() const {
    return get()->GetBufferPointer();
}

size_t Blob::size() const {
    return get()->GetBufferSize();
}

struct SignatureBuilder {
    sm::SmallVector<D3D12_DESCRIPTOR_RANGE1, 4> ranges;
    sm::SmallVector<D3D12_ROOT_PARAMETER1, 4> params;
    sm::SmallVector<D3D12_STATIC_SAMPLER_DESC, 4> samplers;

    size_t add_range(D3D12_DESCRIPTOR_RANGE1 range) {
        ranges.push_back(range);
        return ranges.size() - 1;
    }

    D3D12_ROOT_DESCRIPTOR_TABLE1 add_table_ranges(const sm::Span<const DescriptorRange> &table) {
        size_t start = ranges.size();
        for (auto &each : table) {
            D3D12_DESCRIPTOR_RANGE1 range = {
                .RangeType = each.type.as_facade(),
                .NumDescriptors = each.count,
                .BaseShaderRegister = each.reg,
                .RegisterSpace = each.space,
                .Flags = each.flags.as_facade(),
                .OffsetInDescriptorsFromTableStart = each.offset,
            };
            add_range(range);
        }

        D3D12_ROOT_DESCRIPTOR_TABLE1 result = {
            .NumDescriptorRanges = uint(table.size()),
            .pDescriptorRanges = ranges.data() + start,
        };
        return result;
    }

    size_t add_param(const RootParameter& param) {
        D3D12_ROOT_PARAMETER1 desc = {
            .ParameterType = param.type.as_facade(),
            .ShaderVisibility = param.visibility.as_facade(),
        };
        switch (param.type) {
        case RootParameterType::eRootConsts:
            desc.Constants = {
                .ShaderRegister = param.info.reg,
                .RegisterSpace = param.info.space,
                .Num32BitValues = param.info.count,
            };
            break;
        case RootParameterType::eTable:
            desc.DescriptorTable = add_table_ranges(param.ranges);
            break;
        case RootParameterType::eShaderResource:
        case RootParameterType::eUnorderedAccess:
        case RootParameterType::eConstBuffer:
            desc.Descriptor = {
                .ShaderRegister = param.info.reg,
                .RegisterSpace = param.info.space,
            };
            break;

        default:
            using Reflect = ctu::TypeInfo<RootParameterType>;
            NEVER("unsupported root parameter type %s", Reflect::to_string(param.type).data());
        }

        params.push_back(desc);
        return params.size() - 1;
    }

    size_t add_sampler(const Sampler& sampler) {
        D3D12_STATIC_SAMPLER_DESC desc = {
            .Filter = sampler.filter.as_facade(),
            .AddressU = sampler.address.as_facade(),
            .AddressV = sampler.address.as_facade(),
            .AddressW = sampler.address.as_facade(),
            .MipLODBias = 0.f,
            .MaxAnisotropy = 0,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            .MinLOD = 0.f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = sampler.reg,
            .RegisterSpace = sampler.space,
            .ShaderVisibility = sampler.visibility.as_facade(),
        };

        samplers.push_back(desc);
        return samplers.size() - 1;
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC build_1_1(D3D12_ROOT_SIGNATURE_FLAGS flags) {
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(uint(params.size()), params.data(), uint(samplers.size()), samplers.data(),
                      flags);
        return desc;
    }
};

void RootSignature::create(Context &context, const RootSignatureConfig &config) {
    SignatureBuilder builder;

    for (auto elem : config.params) {
        builder.add_param(elem);
    }

    for (auto elem : config.samplers) {
        builder.add_sampler(elem);
    }

    const auto flags = config.flags.as_facade();
    auto desc = builder.build_1_1(flags);

    Blob signature;
    Blob error;

    const auto version = context.mRootSignatureVersion;
    auto &sink = context.mSink;
    auto &device = context.mDevice;

    if (Result hr = D3DX12SerializeVersionedRootSignature(&desc, version.as_facade(), &signature,
                                                          &error);
        !hr) {
        sink.error("failed to serialize root signature: {}", hr);
        sink.error("error: {}", error.as_string());
        SM_ASSERT_HR(hr);
    }

    SM_ASSERT_HR(device->CreateRootSignature(0, signature.data(), signature.size(),
                                             IID_PPV_ARGS(address())));
}

static D3D12_SHADER_BYTECODE as_bytecode(const ShaderBytecode &bc) {
    return {(const void *)bc.data.data(), bc.data.size_bytes()};
}

void PipelineState::create(Context& context, RootSignature &signature,
                           const PipelineConfig &config) {
    sm::SmallVector<D3D12_INPUT_ELEMENT_DESC, 4> layout;
    for (auto elem : config.input) {
        D3D12_INPUT_ELEMENT_DESC desc = {
            .SemanticName = elem.name,
            .Format = elem.format.as_facade(),
            .AlignedByteOffset = elem.offset,
            .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        };
        layout.push_back(desc);
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
        .pRootSignature = signature.get(),
        .VS = as_bytecode(config.vs),
        .PS = as_bytecode(config.ps),
        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .InputLayout = {layout.data(), uint(layout.size())},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
        .SampleDesc = {1, 0},
    };

    auto& device = context.mDevice;

    SM_ASSERT_HR(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(address())));
}
