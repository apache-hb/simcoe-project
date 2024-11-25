#pragma once

#include "draw/next/context.hpp"
#include "draw/passes/blit.hpp"
#include "draw/passes/vic20.hpp"

namespace sm::draw::next {
    class Vic20DrawContext : public DrawContext {
        using Super = DrawContext;

        Vic20Display *mVic20;
        BlitPass *mBlit;

    public:
        Vic20DrawContext(render::next::ContextConfig config, HWND hwnd, math::uint2 resolution);

        void begin();
        void end();

        D3D12_GPU_DESCRIPTOR_HANDLE getVic20TargetSrv() const noexcept {
            return mBlit->getTargetSrv();
        }

        void write(uint8_t x, uint8_t y, uint8_t colour, uint8_t character) {
            mVic20->write(x, y, colour, character);
        }

        void writeCharacter(uint8_t id, shared::Vic20Character character) {
            mVic20->writeCharacter(id, character);
        }
    };
}