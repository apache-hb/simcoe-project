#include "ui/render.hpp"

#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_barriers.h"
#include "d3dx12/d3dx12_resource_helpers.h"

#include "stb/stb_image_write.h"

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
        {"POSITION", bundle::DataFormat::eRG32_FLOAT, offsetof(ui::Vertex, position)},
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
        .blending = true,
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
    auto& font = m_canvas.get_font();
    auto& rhi = context.get_rhi();
    auto device = rhi.get_device();
    auto& heap = context.get_srv_heap();
    auto& log = context.get_logger();

    const CD3DX12_HEAP_PROPERTIES kDefaultHeapProperties{D3D12_HEAP_TYPE_DEFAULT};

    auto& copy = rhi.open_copy_commands();
    auto& direct = rhi.open_direct_commands();
    auto& atlas = font.get_image();

    const auto kTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        atlas.size.width,
        atlas.size.height,
        1, 1);

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kDefaultHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kTextureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_atlas)
    ));

    const UINT64 kUploadSize = GetRequiredIntermediateSize(m_atlas.get(), 0, 1);
    const auto kUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kUploadSize);

    log.info("font atlas size: {}x{} buffer size {}", atlas.size.width, atlas.size.height, kUploadSize);

    rhi::ResourceObject upload;

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kUploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&upload)
    ));

    const UINT kRowPitch = atlas.size.width * sizeof(math::uint8x4);
    const UINT kSlicePitch = kRowPitch * atlas.size.height;

    const D3D12_SUBRESOURCE_DATA kTextureInfo{
        .pData = atlas.data.data(),
        .RowPitch = kRowPitch,
        .SlicePitch = kSlicePitch,
    };

    UpdateSubresources<1>(copy.get(), m_atlas.get(), upload.get(), 0, 0, 1, &kTextureInfo);

    const auto kIntoTexture = CD3DX12_RESOURCE_BARRIER::Transition(
        m_atlas.get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    direct->ResourceBarrier(1, &kIntoTexture);

    const D3D12_SHADER_RESOURCE_VIEW_DESC kViewDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MipLevels = 1
        },
    };

    m_font_atlas_index = heap.bind_slot();
    device->CreateShaderResourceView(m_atlas.get(), &kViewDesc, heap.cpu_handle(m_font_atlas_index));

    log.info("font atlas index: {}", size_t(m_font_atlas_index));

    rhi.flush_copy_commands(copy);
    rhi.flush_direct_commands(direct);
}

void CanvasCommands::setup_camera(render::Context& context) {
    auto& rhi = context.get_rhi();
    auto& features = rhi.get_features();
    auto& heap = context.get_srv_heap();
    auto device = rhi.get_device();
    auto& log = context.get_logger();

    constexpr UINT kBufferSize = sizeof(CanvasProjection);
    const auto kBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kBufferSize);
    const D3D12_HEAP_PROPERTIES *kProps = features.gpu_upload_heap
        ? &kGpuUploadHeapProperties
        : &kUploadHeapProperties;

    m_camera.try_release();

    SM_ASSERT_HR(device->CreateCommittedResource(
        kProps,
        D3D12_HEAP_FLAG_NONE,
        &kBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_camera)
    ));

    const D3D12_CONSTANT_BUFFER_VIEW_DESC kDesc = {
        .BufferLocation = m_camera->GetGPUVirtualAddress(),
        .SizeInBytes = kBufferSize,
    };

    m_camera_index = heap.bind_slot();
    device->CreateConstantBufferView(&kDesc, heap.cpu_handle(m_camera_index));

    log.info("camera index: {}", size_t(m_camera_index));

    D3D12_RANGE range{0,0};
    SM_ASSERT_HR(m_camera->Map(0, &range, (void**)&m_projection_data));
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
    m_projection.projection = math::float4x4::orthographicRH(0.f, float(size.width), 0.f, -float(size.height), 0.f, 1.f);
    *m_projection_data = m_projection;
}

void CanvasCommands::create(render::Context& context) {
    auto& rhi = context.get_rhi();
    auto size = rhi.get_swapchain_size();

    m_canvas.set_screen(size);
    setup_buffers(context, 0x1000, 0x1000); // create initial buffers
    setup_pipeline(context);
    setup_camera(context);
    update_camera(size);
    setup_font_atlas(context);
}

void CanvasCommands::destroy(render::Context& context) {

}

void CanvasCommands::build(render::Context& context) {
    const auto& [vertices, indices] = m_canvas.get_draw_data();

    if (m_canvas.is_dirty()) {
        setup_buffers(context, vertices.size(), indices.size());

        std::memcpy(m_vbo_data, vertices.data(), vertices.size() * sizeof(ui::Vertex));
        std::memcpy(m_ibo_data, indices.data(), indices.size() * sizeof(ui::Index));

        m_canvas.clear_dirty();
    }

    auto& rhi = context.get_rhi();
    auto& heap = context.get_srv_heap();
    auto& direct = rhi.get_direct_commands();

    direct.set_root_signature(m_pipeline);
    direct.set_pipeline_state(m_pipeline);

    // TODO: really need to not hardcode these offsets
    direct->SetGraphicsRootDescriptorTable(0, heap.gpu_handle(m_font_atlas_index));
    direct->SetGraphicsRootDescriptorTable(1, heap.gpu_handle(m_camera_index));

    // direct->RSSetViewports(1, &m_viewport);
    // direct->RSSetScissorRects(1, &m_scissor);

    constexpr float kBlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    direct->OMSetBlendFactor(kBlendFactor);

    direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    direct->IASetVertexBuffers(0, 1, &m_vbo_view);
    direct->IASetIndexBuffer(&m_ibo_view);

    direct->DrawIndexedInstanced(UINT(indices.size()), 1, 0, 0, 0);
}
