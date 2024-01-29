#include "render/render.hpp"

#include "io/io.h"

#include "d3dx12/d3dx12_barriers.h"
#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_resource_helpers.h"

using namespace sm;
using namespace sm::render;

static sm::Vector<uint8_t> load_shader(const char *file) {
    sm::IArena& arena = sm::get_pool(sm::Pool::eGlobal);
    io_t *io = io_file(file, eAccessRead, &arena);
    io_error_t err = io_error(io);
    CTASSERTF(err == 0, "failed to open shader file %s: %s", file, os_error_string(err, &arena));

    size_t size = io_size(io);
    const void *data = io_map(io, eProtectRead);

    CTASSERTF(size != SIZE_MAX, "failed to get size of shader file %s", file);
    CTASSERTF(data != nullptr, "failed to map shader file %s", file);

    sm::Vector<uint8_t> result(size);
    memcpy(result.data(), data, size);

    io_close(io);

    return result;
}

void Context::setup_pipeline() {
    auto device = m_device.get_device();
    auto version = m_device.get_root_signature_version();

    CD3DX12_DESCRIPTOR_RANGE1 srv_ranges[1];
    srv_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_DESCRIPTOR_RANGE1 cbv_ranges[1];
    cbv_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_ROOT_PARAMETER1 params[2];
    params[0].InitAsDescriptorTable(std::size(srv_ranges), srv_ranges, D3D12_SHADER_VISIBILITY_PIXEL);
    params[1].InitAsDescriptorTable(std::size(cbv_ranges), cbv_ranges, D3D12_SHADER_VISIBILITY_VERTEX);

    const D3D12_STATIC_SAMPLER_DESC kSampler {
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 0,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
        .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
        .MinLOD = 0.0f,
        .MaxLOD = D3D12_FLOAT32_MAX,
        .ShaderRegister = 0,
        .RegisterSpace = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    constexpr D3D12_ROOT_SIGNATURE_FLAGS kRootFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                                                    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
                                                    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
                                                    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsd;
    rsd.Init_1_1(std::size(params), params, 1, &kSampler, kRootFlags);

    rhi::Object<ID3DBlob> signature;
    rhi::Object<ID3DBlob> error;
    if (HRESULT hr = D3DX12SerializeVersionedRootSignature(&rsd, version.as_facade(), &signature, &error);
        FAILED(hr)) {
        m_log.error("failed to serialize root signature: {}", rhi::hresult_string(hr));
        std::string_view err{(char *)error->GetBufferPointer(), error->GetBufferSize()};
        m_log.error("{}", err);
        SM_ASSERT_HR(hr);
    }

    SM_ASSERT_HR(device->CreateRootSignature(0, signature->GetBufferPointer(),
                                               signature->GetBufferSize(),
                                               IID_PPV_ARGS(&m_pipeline.m_signature)));

    constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    auto vs = load_shader("build/client.exe.p/object.vs.cso");
    auto ps = load_shader("build/client.exe.p/object.ps.cso");

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC kPipelineDesc = {
        .pRootSignature = m_pipeline.m_signature.get(),
        .VS = {vs.data(), vs.size()},
        .PS = {ps.data(), ps.size()},

        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .InputLayout = {kInputElements, std::size(kInputElements)},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {rhi::get_data_format(m_config.swapchain_format)},
        .SampleDesc = {1, 0},
    };

    SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kPipelineDesc, IID_PPV_ARGS(&m_pipeline.m_pipeline)));
}

void Context::setup_assets() {
    auto& copy = m_device.open_copy_commands();
    auto& direct = m_device.open_direct_commands();

    auto device = m_device.get_device();
    auto [width, height] = m_device.get_swapchain_size();

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

    const D3D12_RESOURCE_DESC kTextureDesc {
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
        &kDefaultHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kTextureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_texture)
    ));

    const UINT64 kUploadSize = GetRequiredIntermediateSize(m_texture.get(), 0, 1);
    const CD3DX12_RESOURCE_DESC kUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kUploadSize);

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kUploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&texture_upload)
    ));

    constexpr size_t kRowPitch = kTextureWidth * kPixelSize;
    constexpr size_t kSlicePitch = kRowPitch * kTextureHeight;

    const D3D12_SUBRESOURCE_DATA kTextureInfo {
        .pData = kTextureData,
        .RowPitch = kRowPitch,
        .SlicePitch = kSlicePitch,
    };

    UpdateSubresources<1>(copy.get(), m_texture.get(), texture_upload.get(), 0, 0, 1, &kTextureInfo);

    const CD3DX12_RESOURCE_BARRIER kIntoTexture = CD3DX12_RESOURCE_BARRIER::Transition(
        m_texture.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    direct->ResourceBarrier(1, &kIntoTexture);

    const D3D12_SHADER_RESOURCE_VIEW_DESC kViewDesc {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MipLevels = 1
        },
    };

    if (m_texture_index == SrvHeapIndex::eInvalid)
        m_texture_index = m_srv_arena.bind_slot();

    m_log.info("texture index: {}", size_t(m_texture_index));

    device->CreateShaderResourceView(m_texture.get(), &kViewDesc, m_srv_arena.cpu_handle(m_texture_index));

    // create ibo
    constexpr uint16_t kIndexData[] = {0, 1, 2, 2, 1, 3};
    constexpr UINT kIboSize = sizeof(kIndexData);
    const CD3DX12_RESOURCE_DESC kIndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kIboSize);

    m_ibo.try_release();
    rhi::ResourceObject ibo_upload;

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kDefaultHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kIndexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_ibo)
    ));

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kIndexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&ibo_upload)
    ));

    UINT8 *ibo_data;
    CD3DX12_RANGE read_range{0,0};
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
        &kDefaultHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kVertexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_vbo)
    ));

    SM_ASSERT_HR(device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kVertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vbo_upload)
    ));

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

    m_device.flush_copy_commands(copy);
    m_device.flush_direct_commands(direct);

    m_log.info("uploaded assets");
}

void Context::setup_camera() {
    auto device = m_device.get_device();
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
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_camera_resource)
    ));

    const D3D12_CONSTANT_BUFFER_VIEW_DESC kViewDesc {
        .BufferLocation = m_camera_resource->GetGPUVirtualAddress(),
        .SizeInBytes = kBufferSize,
    };

    m_camera_index = m_srv_arena.bind_slot();
    device->CreateConstantBufferView(&kViewDesc, m_srv_arena.cpu_handle(m_camera_index));

    CD3DX12_RANGE read_range{0,0};
    SM_ASSERT_HR(m_camera_resource->Map(0, &read_range, reinterpret_cast<void **>(&m_camera_data)));
}

void Context::update_camera(math::uint2 size) {
    float aspect = float(size.x) / float(size.y);
    m_camera.view = math::float4x4::lookAtRH({1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, kUpVector).transpose();
    m_camera.projection = math::float4x4::perspectiveRH(90.0f * math::kDegToRad<float>, aspect, 0.1f, 100.0f).transpose();
    *m_camera_data = m_camera;
}

void Context::create_node(IGraphNode& node) {
    node.create(*this);
}

void Context::destroy_node(IGraphNode& node) {
    node.destroy(*this);
}

void Context::execute_inner(IGraphNode& node) {
    if (m_executed.contains(&node))
        return;

    for (auto& input : node.m_inputs) {
        auto it = m_edges.find(input.get());
        if (it == m_edges.end())
            continue;

        auto& output = *it->second;
        execute_inner(output);
    }

    node.build(*this);
    m_executed.emplace(&node, true);
}

void Context::execute_node(IGraphNode& node) {
    m_executed.clear();
    execute_inner(node);
}

void Context::connect(NodeInput* dst, IGraphNode* src) {
    m_edges.emplace(dst, src);
}

void Context::resize(math::uint2 size) {
    auto current = m_device.get_swapchain_size();

    if (current == size)
        return;

    m_device.resize(size);

    update_camera(size);
}

void Context::begin_frame() {
    update_camera(m_device.get_swapchain_size());

    auto& commands = m_device.open_direct_commands(m_pipeline.m_pipeline.get());
    m_device.begin_frame();

    ID3D12DescriptorHeap *heaps[] = {m_srv_arena.get_heap()};
    commands->SetDescriptorHeaps(std::size(heaps), heaps);

    commands->SetGraphicsRootSignature(m_pipeline.m_signature.get());

    // use 2 tables in case the indices are not contiguous
    commands->SetGraphicsRootDescriptorTable(0, m_srv_arena.gpu_handle(m_texture_index));
    commands->SetGraphicsRootDescriptorTable(1, m_srv_arena.gpu_handle(m_camera_index));
    commands->RSSetViewports(1, &m_viewport);
    commands->RSSetScissorRects(1, &m_scissor);

    commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commands->IASetVertexBuffers(0, 1, &m_vbo_view);
    commands->IASetIndexBuffer(&m_ibo_view);
    commands->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Context::end_frame() {
    m_device.end_frame();
}

void Context::present() {
    m_device.present();
}

Context::Context(RenderConfig config, rhi::Factory& factory)
    : m_config(config)
    , m_log(config.logger)
    , m_factory(factory)
    , m_device(factory.new_device())
{
    rhi::DescriptorHeapConfig cbv_heap_config = {
        .type = rhi::DescriptorHeapType::eCBV_SRV_UAV,
        .size = config.cbv_heap_size,
        .shader_visible = true,
    };

    m_srv_arena.init(m_device, cbv_heap_config);

    rhi::DescriptorHeapConfig rtv_heap_config = {
        .type = rhi::DescriptorHeapType::eRTV,
        .size = config.rtv_heap_size,
        .shader_visible = false,
    };

    m_rtv_arena.init(m_device, rtv_heap_config);

    setup_pipeline();
    setup_assets();
    setup_camera();
}

Context::~Context() {
    for (auto& node : m_nodes) {
        destroy_node(*node.get());
    }

    m_nodes.clear();
}
