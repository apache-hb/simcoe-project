#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

void ICommandBundle::bind_resources(Context& context) {
    SM_UNUSED constexpr auto refl = ctu::reflect<rhi::DescriptorHeapType>();
    for (auto& binding : m_bindings) {
        switch (binding->heap) {
        case rhi::DescriptorHeapType::eCBV_SRV_UAV:
            binding->srv = context.get_srv_heap().bind_slot();
            break;
        case rhi::DescriptorHeapType::eRTV:
            binding->rtv = context.get_rtv_heap().bind_slot();
            break;
        default:
            NEVER("Invalid descriptor heap type %s", refl.to_string(binding->heap).data());
        }
    }
}

void ICommandBundle::unbind_resources(Context& context) {
    SM_UNUSED constexpr auto refl = ctu::reflect<rhi::DescriptorHeapType>();
    for (auto& binding : m_bindings) {
        switch (binding->heap) {
        case rhi::DescriptorHeapType::eCBV_SRV_UAV:
            context.get_srv_heap().unbind_slot(binding->srv);
            break;
        case rhi::DescriptorHeapType::eRTV:
            context.get_rtv_heap().unbind_slot(binding->rtv);
            break;
        default:
            NEVER("Invalid descriptor heap type %s", refl.to_string(binding->heap).data());
        }
    }
}

void Context::create_node(ICommandBundle& node) {
    node.bind_resources(*this);
    node.create(*this);
}

void Context::destroy_node(ICommandBundle& node) {
    node.destroy(*this);
    node.unbind_resources(*this);
}

void Context::execute_node(ICommandBundle& node) {
    node.build(*this);
}

Context::Context(RenderConfig config, rhi::Factory& factory)
    : m_config(config)
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
        destroy_node(*node.get());
    }

    m_nodes.clear();
}
