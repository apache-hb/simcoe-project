#pragma once

#include "draw/next/context.hpp"

#include "draw/passes/blit.hpp"

namespace game {
    class WorldDrawContext final : public sm::draw::next::DrawContext {
        using Super = sm::draw::next::DrawContext;

        sm::draw::next::BlitPass *mBlit;

    public:
        WorldDrawContext(sm::render::next::ContextConfig config, HWND hwnd, sm::math::uint2 resolution);

        void begin();
        void end();
    };
}
