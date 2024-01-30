#include "ui/render.hpp"

#include "d3dx12/d3dx12_core.h"

using namespace sm;
using namespace sm::ui;

static_assert(sizeof(ui::Index) == sizeof(uint16_t), "index size mismatch");

static constexpr D3D12_HEAP_PROPERTIES kUploadHeapProperties = {
    .Type = D3D12_HEAP_TYPE_UPLOAD,
};

static constexpr D3D12_HEAP_PROPERTIES kGpuUploadHeapProperties = {
    .Type = D3D12_HEAP_TYPE_GPU_UPLOAD,
};

void CanvasCommands::setup_pipeline(render::Context& context) {
    auto& rhi = context.get_rhi();
    static constexpr rhi::InputElement kInputLayout[] = {
        {"POSITION", bundle::DataFormat::eRGB32_FLOAT, offsetof(ui::Vertex, position)},
        {"TEXCOORD", bundle::DataFormat::eRG32_FLOAT, offsetof(ui::Vertex, uv)},
        {"COLOUR", bundle::DataFormat::eRGBA8_UNORM, offsetof(ui::Vertex, colour)},
    };

    static constexpr rhi::StaticSampler kStaticSamplers[] = {
        {.filter = rhi::Filter::eMinMagMipLinear, .address = rhi::AddressMode::eBorder},
    };

    static constexpr rhi::ResourceBinding kShaderBindings[] = {
        {kFontAtlasSlot, rhi::ShaderVisibility::ePixel, rhi::BindingType::eTexture, 0, 0},
        {kCameraSlot, rhi::ShaderVisibility::eVertex, rhi::BindingType::eUniform, 0, 0},
    };

    auto vs = m_assets.load_shader(kVertexShader);
    auto ps = m_assets.load_shader(kPixelShader);

    const rhi::GraphicsPipelineConfig kPipelineConfig = {
        .elements = {kInputLayout, std::size(kInputLayout)},
        .resources = {kShaderBindings, std::size(kShaderBindings)},
        .samplers = {kStaticSamplers, std::size(kStaticSamplers)},
        .vs = { vs.data(), vs.size() },
        .ps = { ps.data(), ps.size() },
    };

    m_pipeline.init(rhi, kPipelineConfig);
}

void CanvasCommands::setup_buffers(render::Context& context, uint32_t vbo_length, uint32_t ibo_length) {
    auto& rhi = context.get_rhi();
    auto& features = rhi.get_features();
    auto device = rhi.get_device();

    // using rebar is a little overkill here
    const D3D12_HEAP_PROPERTIES *kProps = features.gpu_upload_heap
        ? &kGpuUploadHeapProperties
        : &kUploadHeapProperties;

    if (vbo_length > m_vbo_length) {
        m_vbo_length = vbo_length;

        /// note: did some digging on msdn, looks like
        ///       you dont need to unmap before releasing. (might be wrong though)

        m_vbo.try_release();

        const auto kBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vbo_length * sizeof(ui::Vertex));

        SM_ASSERT_HR(device->CreateCommittedResource(
            kProps,
            D3D12_HEAP_FLAG_NONE,
            &kBufferDesc,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            IID_PPV_ARGS(&m_vbo)
        ));

        m_vbo_view = {
            .BufferLocation = m_vbo->GetGPUVirtualAddress(),
            .SizeInBytes = UINT(vbo_length * sizeof(ui::Vertex)),
            .StrideInBytes = sizeof(ui::Vertex),
        };

        D3D12_RANGE range{0,0};
        SM_ASSERT_HR(m_vbo->Map(0, &range, (void**)&m_vbo_data));
    }

    CTASSERTF(ibo_length <= UINT16_MAX, "ibo length too large %u", ibo_length);

    if (ibo_length > m_ibo_length) {
        m_ibo_length = ibo_length;

        m_ibo.try_release();

        const auto kBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(ibo_length * sizeof(ui::Index));

        SM_ASSERT_HR(device->CreateCommittedResource(
            kProps,
            D3D12_HEAP_FLAG_NONE,
            &kBufferDesc,
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            nullptr,
            IID_PPV_ARGS(&m_ibo)
        ));

        m_ibo_view = {
            .BufferLocation = m_ibo->GetGPUVirtualAddress(),
            .SizeInBytes = UINT(ibo_length * sizeof(ui::Index)),
            .Format = DXGI_FORMAT_R16_UINT,
        };

        D3D12_RANGE range{0,0};
        SM_ASSERT_HR(m_ibo->Map(0, &range, (void**)&m_ibo_data));
    }
}

void CanvasCommands::setup_font_atlas(render::Context& context) {

}

void CanvasCommands::setup_camera(render::Context& context) {

}

void CanvasCommands::update_viewport(math::uint2 size) {
    auto [width, height] = size;

    m_viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = float(width),
        .Height = float(height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };

    m_scissor = {
        .left = 0,
        .top = 0,
        .right = LONG(width),
        .bottom = LONG(height),
    };
}

void CanvasCommands::update_camera(math::uint2 size) {
    m_projection = math::float4x4::orthographicRH(float(size.width), float(size.height), 0.f, 1.f);
}

void CanvasCommands::create(render::Context& context) {
    setup_buffers(context, 0x1000, 0x1000); // create initial buffers
}

void CanvasCommands::destroy(render::Context& context) {

}

void CanvasCommands::build(render::Context& context) {
    if (!m_canvas.is_dirty()) return;

    // m_canvas.layout(m_draw, tmp);

    // setup_buffers(context, m_draw.vertices.size(), m_draw.indices.size());

    // std::copy_n(m_draw.vertices.data(), m_draw.vertices.size(), m_vbo_data);
    // std::copy_n(m_draw.indices.data(), m_draw.indices.size(), m_ibo_data);
}
