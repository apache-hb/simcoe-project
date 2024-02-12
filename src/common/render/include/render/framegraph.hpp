#pragma once

#include "render.reflect.h"

namespace sm::render {
    class IGraphResource;

    class PassBuilder {
    public:
        template<typename T>
        T *create(auto&&... args);

        void write(const IGraphResource &resource);
        void read(const IGraphResource &resource);

        void side_effects();
    };

    class IGraphResource {
        ResourceKind m_type;
        unsigned m_id;
        unsigned m_version;

    public:
        constexpr IGraphResource(ResourceKind type)
            : m_type(type)
        { }

        constexpr ResourceKind get_type() const { return m_type; }
        constexpr bool is_persistent() const { return get_type() == ResourceKind::ePersistent; }
        constexpr bool is_transient() const { return get_type() == ResourceKind::eTransient; }

        virtual void before_pass() = 0;
        virtual void after_pass() = 0;
    };

    class RenderPass {

    };

    class FrameGraph {
    public:
        template<typename T>
        T *create(auto&&... args);

        template<typename T>
        T *imports(auto&&... args);
    };
}
