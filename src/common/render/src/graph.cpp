#include "render/graph.hpp"

#include "d3dx12/d3dx12_barriers.h"
#include "d3dx12/d3dx12_resource_helpers.h"

using namespace sm;
using namespace sm::render;

void WorldCommands::update_viewport() {
    auto [width, height] = m_size;

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

void WorldCommands::setup_pipeline(render::Context &ctx) {
    auto &rhi = ctx.get_rhi();

    static constexpr rhi::InputElement kInputLayout[] = {
        {"POSITION", bundle::DataFormat::eRGB32_FLOAT, offsetof(Vertex, position)},
        {"TEXCOORD", bundle::DataFormat::eRG32_FLOAT, offsetof(Vertex, uv)},
    };

    static constexpr rhi::StaticSampler kStaticSamplers[] = {
        {.filter = rhi::Filter::eMinMagMipLinear, .address = rhi::AddressMode::eBorder},
    };

    static constexpr rhi::ResourceBinding kShaderBindings[] = {
        {kTextureSlot, rhi::ShaderVisibility::ePixel, rhi::BindingType::eTexture, 0, 0},
        {kCameraSlot, rhi::ShaderVisibility::eVertex, rhi::BindingType::eUniform, 0, 0},
    };

    auto vs = m_assets.load_shader(kVertexShader);
    auto ps = m_assets.load_shader(kPixelShader);

    const rhi::GraphicsPipelineConfig kPipelineConfig = {
        .elements = {kInputLayout, std::size(kInputLayout)},
        .resources = {kShaderBindings, std::size(kShaderBindings)},
        .samplers = {kStaticSamplers, std::size(kStaticSamplers)},
        .vs = {vs.data(), vs.size()},
        .ps = {ps.data(), ps.size()},
    };

    m_pipeline.init(rhi, kPipelineConfig);
}

void WorldCommands::setup_assets(render::Context &ctx) {
    auto &rhi = ctx.get_rhi();
    auto &heap = ctx.get_srv_heap();
    auto &log = ctx.get_logger();
    auto device = rhi.get_device();

    auto &copy = rhi.open_copy_commands();
    auto &direct = rhi.open_direct_commands();

    const CD3DX12_HEAP_PROPERTIES kDefaultHeapProperties{D3D12_HEAP_TYPE_DEFAULT};
    const CD3DX12_HEAP_PROPERTIES kUploadHeapProperties{D3D12_HEAP_TYPE_UPLOAD};

    // create and upload texture

    constexpr size_t kTextureWidth = 2;
    constexpr size_t kTextureHeight = 2;
    constexpr size_t kPixelSize = sizeof(math::uint8x4);

    constexpr math::uint8x4 kTextureData[kTextureWidth * kTextureHeight] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 255, 255},
    };

    rhi::ResourceObject texture_upload;

    const D3D12_RESOURCE_DESC kTextureDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Width = kTextureWidth,
        .Height = kTextureHeight,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {1, 0},
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kDefaultHeapProperties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &kTextureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_texture)));

    const UINT64 kUploadSize = GetRequiredIntermediateSize(m_texture.get(), 0, 1);
    const CD3DX12_RESOURCE_DESC kUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kUploadSize);

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kUploadHeapProperties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &kUploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&texture_upload)));

    constexpr size_t kRowPitch = kTextureWidth * kPixelSize;
    constexpr size_t kSlicePitch = kRowPitch * kTextureHeight;

    const D3D12_SUBRESOURCE_DATA kTextureInfo{
        .pData = kTextureData,
        .RowPitch = kRowPitch,
        .SlicePitch = kSlicePitch,
    };

    UpdateSubresources<1>(copy.get(), m_texture.get(), texture_upload.get(), 0, 0, 1,
                          &kTextureInfo);

    const CD3DX12_RESOURCE_BARRIER kIntoTexture = CD3DX12_RESOURCE_BARRIER::Transition(
        m_texture.get(), D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    direct->ResourceBarrier(1, &kIntoTexture);

    const D3D12_SHADER_RESOURCE_VIEW_DESC kViewDesc{
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {.MipLevels = 1},
    };

    if (m_texture_index == SrvHeapIndex::eInvalid) m_texture_index = heap.bind_slot();

    log.info("texture index: {}", size_t(m_texture_index));

    device->CreateShaderResourceView(m_texture.get(), &kViewDesc, heap.cpu_handle(m_texture_index));

    // create ibo
    constexpr uint16_t kIndexData[] = {0, 1, 2, 2, 1, 3};
    constexpr UINT kIboSize = sizeof(kIndexData);
    const CD3DX12_RESOURCE_DESC kIndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kIboSize);

    m_ibo.try_release();
    rhi::ResourceObject ibo_upload;

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kDefaultHeapProperties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &kIndexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_ibo)));

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kUploadHeapProperties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &kIndexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ibo_upload)));

    UINT8 *ibo_data;
    CD3DX12_RANGE read_range{0, 0};
    SM_ASSERT_HR(ibo_upload->Map(0, &read_range, reinterpret_cast<void **>(&ibo_data)));
    memcpy(ibo_data, kIndexData, kIboSize);
    ibo_upload->Unmap(0, nullptr);

    const CD3DX12_RESOURCE_BARRIER kIntoIbo = CD3DX12_RESOURCE_BARRIER::Transition(
        m_ibo.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    copy->CopyBufferRegion(m_ibo.get(), 0, ibo_upload.get(), 0, kIboSize);
    direct->ResourceBarrier(1, &kIntoIbo);

    m_ibo_view = {
        .BufferLocation = m_ibo->GetGPUVirtualAddress(),
        .SizeInBytes = kIboSize,
        .Format = DXGI_FORMAT_R16_UINT,
    };

    // create vbo
    constexpr Vertex kVertexData[] = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},
    };

    constexpr UINT kVboSize = sizeof(kVertexData);

    const CD3DX12_RESOURCE_DESC kVertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kVboSize);

    // upload vbo data
    m_vbo.try_release();
    rhi::ResourceObject vbo_upload;

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kDefaultHeapProperties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &kVertexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vbo)));

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kUploadHeapProperties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &kVertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vbo_upload)));

    UINT8 *vbo_data;
    SM_ASSERT_HR(vbo_upload->Map(0, &read_range, reinterpret_cast<void **>(&vbo_data)));
    memcpy(vbo_data, kVertexData, kVboSize);
    vbo_upload->Unmap(0, nullptr);

    const CD3DX12_RESOURCE_BARRIER kIntoBuffer = CD3DX12_RESOURCE_BARRIER::Transition(
        m_vbo.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    copy->CopyBufferRegion(m_vbo.get(), 0, vbo_upload.get(), 0, kVboSize);
    direct->ResourceBarrier(1, &kIntoBuffer);

    m_vbo_view = {
        .BufferLocation = m_vbo->GetGPUVirtualAddress(),
        .SizeInBytes = kVboSize,
        .StrideInBytes = sizeof(Vertex),
    };

    rhi.flush_copy_commands(copy);
    rhi.flush_direct_commands(direct);

    log.info("uploaded assets");
}

void WorldCommands::setup_camera(render::Context &ctx) {
    auto &rhi = ctx.get_rhi();
    auto device = rhi.get_device();
    auto &heap = ctx.get_srv_heap();
    CameraBuffer canera = {
        .model = math::float4x4::identity(),
        .view = math::float4x4::identity(),
        .projection = math::float4x4::identity(),
    };

    m_camera = canera;

    constexpr UINT kBufferSize = sizeof(CameraBuffer);
    const CD3DX12_HEAP_PROPERTIES kUploadHeapProperties{D3D12_HEAP_TYPE_UPLOAD};
    const CD3DX12_RESOURCE_DESC kBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kBufferSize);

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kUploadHeapProperties, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &kBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_camera_resource)));

    const D3D12_CONSTANT_BUFFER_VIEW_DESC kViewDesc{
        .BufferLocation = m_camera_resource->GetGPUVirtualAddress(),
        .SizeInBytes = kBufferSize,
    };

    m_camera_index = heap.bind_slot();
    device->CreateConstantBufferView(&kViewDesc, heap.cpu_handle(m_camera_index));

    CD3DX12_RANGE read_range{0, 0};
    SM_ASSERT_HR(m_camera_resource->Map(0, &read_range, reinterpret_cast<void **>(&m_camera_data)));
}

void WorldCommands::update_camera() {
    float aspect = float(m_size.x) / float(m_size.y);
    m_camera.view =
        math::float4x4::lookAtRH({1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, kUpVector).transpose();
    m_camera.projection = math::float4x4::perspectiveRH(90.0f * math::kDegToRad<float>, aspect,
                                                        0.1f, 100.0f)
                              .transpose();
    *m_camera_data = m_camera;
}
