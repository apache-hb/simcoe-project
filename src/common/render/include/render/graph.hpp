#pragma once

#include "render/render.hpp"

namespace sm::render {
    class BeginCommands : public render::IRenderNode {

    public:
        void build(render::Context& ctx) override {
            ctx.begin_frame();
        }
    };

    class EndCommands : public render::IRenderNode {

    public:
        void build(render::Context& ctx) override {
            ctx.end_frame();
        }
    };

    class WorldCommands : public render::IRenderNode {

    public:
        void build(render::Context& ctx) override {
            // auto& commands = ctx.get_direct_commands();
        }
    };

    class PresentCommands : public render::IRenderNode {
    public:
        void build(render::Context& ctx) override {
            ctx.present();
        }
    };
}
