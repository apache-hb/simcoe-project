#pragma once

#include <concepts>

#include <directx/d3d12.h>

namespace sm::graph {
    class Resource {

    };

    class TextureCreateInfo {

    };

    class Attachment {

    };

    struct RenderPassInfo {

    };

    class RenderPass {
    protected:
        Attachment read(Resource& resource);
        Attachment write(Resource& resource);
        Resource create(TextureCreateInfo info);

    public:
        virtual ~RenderPass() = default;

        RenderPass(const RenderPassInfo& info, D3D12_COMMAND_LIST_TYPE type);

        virtual void create() { }
        virtual void destroy() { }
        virtual void execute() { }
    };

    class FrameGraph {
        void addRenderPass(RenderPass *pass);

    public:
        FrameGraph() = default;

        template<std::derived_from<RenderPass> T>
        T *add(auto&&... args) {

        }
    };
}
