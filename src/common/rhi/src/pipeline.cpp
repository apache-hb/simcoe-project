#include "rhi/rhi.hpp"

#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_root_signature.h"

using namespace sm;
using namespace sm::rhi;

struct PipelineBuilder {
    sm::Vector<D3D12_STATIC_SAMPLER_DESC> samplers;
    sm::Vector<D3D12_INPUT_ELEMENT_DESC> input_elements;

    sm::Vector<D3D12_DESCRIPTOR_RANGE1> ranges;
    sm::Vector<D3D12_ROOT_PARAMETER1> params;

    constexpr D3D12_STATIC_SAMPLER_DESC build_sampler(const rhi::StaticSampler &config) {
        const D3D12_TEXTURE_ADDRESS_MODE kMode = config.address.as_facade();
        const D3D12_STATIC_SAMPLER_DESC kSampler = {
            .Filter = config.filter.as_facade(),
            .AddressU = kMode,
            .AddressV = kMode,
            .AddressW = kMode,
            .MipLODBias = 0.f,
            .MaxAnisotropy = 0,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            .MinLOD = 0.f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = config.reg,
            .RegisterSpace = 0,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
        };

        return kSampler;
    }

    constexpr D3D12_INPUT_ELEMENT_DESC build_element(const rhi::InputElement &config) {
        const DXGI_FORMAT kFormat = rhi::get_data_format(config.format.as_enum());
        const D3D12_INPUT_ELEMENT_DESC kElement = {
            .SemanticName = config.name,
            .SemanticIndex = 0,
            .Format = kFormat,
            .InputSlot = 0,
            .AlignedByteOffset = config.offset,
            .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };

        return kElement;
    }

    constexpr void add_sampler(const rhi::StaticSampler &config) {
        samplers.push_back(build_sampler(config));
    }

    constexpr void add_element(const rhi::InputElement &config) {
        input_elements.push_back(build_element(config));
    }

    constexpr void add_binding(const rhi::ResourceBinding &config) {
        const D3D12_DESCRIPTOR_RANGE1 kRange = {
            .RangeType = config.binding.get_range_type(),
            .NumDescriptors = 1,
            .BaseShaderRegister = config.reg,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
        };

        auto &it = ranges.emplace_back(kRange);

        const D3D12_ROOT_PARAMETER1 kParam = {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable =
                {
                    .NumDescriptorRanges = 1,
                    .pDescriptorRanges = &it,
                },
            .ShaderVisibility = config.visibility.as_facade(),
        };

        params.emplace_back(kParam);
    }

    PipelineBuilder(const GraphicsPipelineConfig &config) {
        samplers.reserve(config.samplers.count);
        input_elements.reserve(config.elements.count);
        ranges.reserve(config.resources.count);
        params.reserve(config.resources.count);
    }
};

void PipelineState::init(Device &device, const GraphicsPipelineConfig &config) {
    PipelineBuilder builder{config};

    auto &in_samplers = config.samplers;
    auto &in_elements = config.elements;
    auto &in_binding = config.resources;

    for (unsigned i = 0; i < in_samplers.count; i++) {
        builder.add_sampler(in_samplers.samplers[i]);
    }

    for (unsigned i = 0; i < in_elements.count; i++) {
        builder.add_element(in_elements.elements[i]);
    }

    for (unsigned i = 0; i < in_binding.count; i++) {
        builder.add_binding(in_binding.bindings[i]);
    }

    SM_UNUSED auto sampler_count = builder.samplers.size();
    SM_UNUSED auto element_count = builder.input_elements.size();
    SM_UNUSED auto binding_count = builder.ranges.size();
    SM_UNUSED auto param_count = builder.params.size();

    CTASSERTF(sampler_count == in_samplers.count, "samplers mismatch %zu != %u", sampler_count,
              in_samplers.count);
    CTASSERTF(element_count == in_elements.count, "elements mismatch %zu != %u", element_count,
              in_elements.count);
    CTASSERTF(binding_count == in_binding.count, "ranges mismatch %zu != %u", binding_count,
              in_binding.count);
    CTASSERTF(param_count == in_binding.count, "params mismatch %zu != %u", param_count,
              in_binding.count);

    constexpr D3D12_ROOT_SIGNATURE_FLAGS kFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    desc.Init_1_1(param_count, builder.params.data(), sampler_count, builder.samplers.data(),
                  kFlags);

    rhi::Object<ID3DBlob> signature;
    rhi::Object<ID3DBlob> error;

    auto version = device.get_root_signature_version();
    auto &log = device.get_logger();
    auto &device_info = device.get_config();

    if (HRESULT hr = D3DX12SerializeVersionedRootSignature(&desc, version.as_facade(), &signature,
                                                           &error);
        FAILED(hr)) {
        log.error("failed to serialize root signature: {}", rhi::hresult_string(hr));
        std::string_view err{(char *)error->GetBufferPointer(), error->GetBufferSize()};
        log.error("{}", err);
        SM_ASSERT_HR(hr);
    }

    auto dev = device.get_device();

    SM_ASSERT_HR(dev->CreateRootSignature(0, signature->GetBufferPointer(),
                                          signature->GetBufferSize(), IID_PPV_ARGS(&m_signature)));

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC kPipelineDesc = {
        .pRootSignature = m_signature.get(),
        .VS = {config.vs.data, config.vs.size},
        .PS = {config.ps.data, config.ps.size},

        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        // no depth stencil state for now
        .InputLayout = {builder.input_elements.data(), (UINT)builder.input_elements.size()},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {rhi::get_data_format(device_info.buffer_format)},
        .SampleDesc = {1, 0},
    };

    SM_ASSERT_HR(dev->CreateGraphicsPipelineState(&kPipelineDesc, IID_PPV_ARGS(&m_pipeline)));
}
