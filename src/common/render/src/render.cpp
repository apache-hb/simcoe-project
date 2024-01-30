#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

void Context::create_node(IGraphNode& node) {
    node.create(*this);
    m_nodes.emplace_back(&node);
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

    for (auto& node : m_nodes) {
        node->resize(*this, size);
    }
}

void Context::begin_frame() {
    m_device.open_direct_commands();
    m_device.begin_frame();
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
}

Context::~Context() {
    for (auto& node : m_nodes) {
        node->destroy(*this);
    }

    m_nodes.clear();
}
