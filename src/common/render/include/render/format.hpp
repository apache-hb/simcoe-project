#pragma once

#include <directx/d3d12.h>

#include <fmtlib/format.h>

namespace sm::render {
    sm::String format(D3D12_RESOURCE_STATES states);
}

template<>
struct fmt::formatter<D3D12_RESOURCE_STATES> {
    constexpr auto parse(fmt::format_parse_context& ctx) const { return ctx.begin(); }

    constexpr auto format(D3D12_RESOURCE_STATES states, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", sm::render::format(states));
    }
};
