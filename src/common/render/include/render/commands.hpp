#pragma once

#include "core/vector.hpp"

#include "render/resource.hpp"

namespace sm::render {
    class IGraphWire {
    public:
        virtual ~IGraphWire() = default;
    };

    class IPassInput : public IGraphWire {
        friend class IRenderPass;
        virtual void update_input(Context& context) = 0;

    public:
        IResource *get(Context& context);
    };

    class IPassOutput : public IGraphWire {
        friend class IRenderPass;
        virtual void update_output(Context& context) = 0;

    public:
        void set(IResource *resource);
    };

    template<typename T>
    class In : public IPassInput {
        void update_input(Context& context) override;
    };

    template<typename T>
    class Out : public IPassOutput {
        void update_output(Context& context) override;
    };

    template<typename T>
    class InOut : public IPassInput, public IPassOutput {
        IResource *m_resource;

        void update_input(Context& context) override;
        void update_output(Context& context) override;
    };

    class IRenderPass {
        friend class Context;

        Context& m_context;

        sm::Vector<IPassInput*> m_inputs;
        sm::Vector<IPassOutput*> m_outputs;

        void add_input(IPassInput *input);
        void add_output(IPassOutput *output);

    protected:
        IRenderPass(Context& context);

        template<typename T>
        In<T>& in(rhi::ResourceState state) {
            In<T> *it = new In<T>();
            add_input(it);
            return *it;
        }

        template<typename T>
        Out<T>& out(rhi::ResourceState state) {
            Out<T> *it = new Out<T>();
            add_output(it);
            return *it;
        }

        template<typename T>
        InOut<T>& inout(rhi::ResourceState expects, rhi::ResourceState provides) {
            InOut<T> *it = new InOut<T>();
            add_input(it);
            add_output(it);
            return *it;
        }

        virtual void execute() = 0;

        virtual void create() { }
        virtual void destroy() { }

    public:
        virtual ~IRenderPass() = default;

        void build();
    };

    class RenderGraph {
        sm::Vector<IRenderPass*> m_passes;
    };
}
