#pragma once

#include "rhi/rhi.hpp"

#include "render.reflect.h"

namespace sm::render {
    template<typename T>
    concept VertexInput = requires {
        { T::kInputLayout } -> std::convertible_to<rhi::InputLayout>;
    };

    class IResource {
        friend class Context;

        ResourceType m_type;

        virtual rhi::ResourceObject& get() = 0;

        virtual void create(Context& context) = 0;
        virtual void destroy(Context& context) = 0;

    protected:
        constexpr IResource(ResourceType type)
            : m_type(type)
        { }

    public:
        virtual ~IResource() = default;

        constexpr ResourceType get_type() const { return m_type; }
    };

    class IConstBuffer : public IResource {
        rhi::ResourceObject m_resource;
        void *m_pointer = nullptr;
        size_t size;

    protected:
        IConstBuffer(ResourceType type, rhi::ResourceObject&& resource, size_t size)
            : IResource(type)
            , m_resource(std::move(resource))
            , size(size)
        { }

        void map_resource();
        void unmap_resource();
        void update_resource(const void *data, size_t size);
    };

    template<typename T> requires (alignof(T) >= CBUFFER_ALIGN)
    class ConstBuffer : public IConstBuffer {

    };

    class IRenderTarget : public IResource {
        math::float4 m_clear_colour;

    protected:
        IRenderTarget(ResourceType type, math::float4 clear_colour)
            : IResource(type)
            , m_clear_colour(clear_colour)
        { }

    public:
        constexpr math::float4 get_clear_colour() const { return m_clear_colour; }
    };

    class IDepthStencil : public IResource {
        float m_clear_depth;
        uint8_t m_clear_stencil;

    protected:
        IDepthStencil(ResourceType type, float clear_depth, uint8_t clear_stencil)
            : IResource(type)
            , m_clear_depth(clear_depth)
            , m_clear_stencil(clear_stencil)
        { }

    public:
        constexpr float get_clear_depth() const { return m_clear_depth; }
        constexpr uint8_t get_clear_stencil() const { return m_clear_stencil; }
    };

    class ITexture : public IResource {

    };

    class IVertexBuffer : public IResource {
        rhi::ResourceObject m_resource;
        rhi::InputLayout m_layout;

    public:
        constexpr const rhi::InputLayout& get_layout() const { return m_layout; }
    };

    template<VertexInput T>
    class VertexBuffer : public IVertexBuffer {
        void destroy(Context& context) override;
        void create(Context& context) override;

        rhi::ResourceObject &get() override;
    };

    class IIndexBuffer : public IResource {
        rhi::ResourceObject m_resource;
        bundle::DataFormat m_format;

    public:
        constexpr bundle::DataFormat get_format() const { return m_format; }
    };

    template<typename T>
    class IndexBuffer : public IIndexBuffer {
        void destroy(Context& context) override;
        void create(Context& context) override;

        rhi::ResourceObject &get() override;
    };

    class SwapChain : public IResource {
    public:
        void present();
    };
}
