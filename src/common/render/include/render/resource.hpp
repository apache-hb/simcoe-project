#pragma once

#include "rhi/rhi.hpp"

#include "render.reflect.h"

namespace sm::render {
    template<typename T>
    concept VertexInput = requires {
        { T::kInputLayout } -> std::convertible_to<rhi::InputLayout>;
    };

    template<typename T>
    concept IndexInput = requires {
        { T::kIndexType } -> std::convertible_to<bundle::DataFormat>;
    };

    class IResource {
        ResourceType m_type;

    protected:
        IResource(ResourceType type) : m_type(type) { }

    public:
        virtual ~IResource() = default;

        constexpr ResourceType get_type() const { return m_type; }

        virtual rhi::ResourceObject get() = 0;
    };

    class IConstBuffer : public IResource {
        rhi::ResourceObject m_resource;
        void *m_pointer = nullptr;
        size_t size;

    protected:
        IConstBuffer(ResourceType type, size_t size)
            : IResource(type)
            , size(size)
        { }

        void map_resource(rhi::ResourceObject resource);
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

    class IVertexBuffer : public IResource {
        rhi::ResourceObject m_resource;
        rhi::InputLayout m_layout;

    public:
        constexpr const rhi::InputLayout& get_layout() const { return m_layout; }
    };

    template<VertexInput T>
    class VertexBuffer : public IVertexBuffer {

    };

    class IIndexBuffer : public IResource {
        rhi::ResourceObject m_resource;
        bundle::DataFormat m_format;

    public:
        constexpr bundle::DataFormat get_format() const { return m_format; }
    };

    template<IndexInput T>
    class IndexBuffer : public IIndexBuffer {

    };
}
