#pragma once

#include "render/commands.hpp"

#include "render/graph.hpp"

namespace sm::render {
    class WorldPass : public IRenderPass {
    public:
        InOut<IRenderTarget> &rtv = inout<IRenderTarget>(rhi::ResourceState::eRenderTarget, rhi::ResourceState::eRenderTarget);
        InOut<IDepthStencil> &dsv = inout<IDepthStencil>(rhi::ResourceState::eDepthWrite, rhi::ResourceState::eDepthWrite);

    private:
        void execute() override;
    };

    class PostPass : public IRenderPass {
    public:
        In<ITexture> &image = in<ITexture>(rhi::ResourceState::eTextureRead);
        InOut<IRenderTarget> &swapchain = inout<IRenderTarget>(rhi::ResourceState::eRenderTarget, rhi::ResourceState::eRenderTarget);

    private:
        VertexBuffer<Vertex> m_quad;
        IndexBuffer<uint16_t> m_indices;

        void execute() override;
    };

    class PresentPass : public IRenderPass {
    public:
        In<SwapChain> &swapchain = in<SwapChain>(rhi::ResourceState::ePresent);

    private:
        void execute() override;
    };
}
