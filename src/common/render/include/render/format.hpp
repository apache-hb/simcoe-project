#pragma once

#include <directx/d3d12.h>

#include <fmtlib/format.h>

namespace sm::render {
    sm::String format(D3D12_RESOURCE_STATES states);

    template<typename T>
    consteval DXGI_FORMAT bufferFormatOf() { return DXGI_FORMAT_UNKNOWN; }

    template<> consteval DXGI_FORMAT bufferFormatOf<float>() { return DXGI_FORMAT_R32_FLOAT; }
    template<> consteval DXGI_FORMAT bufferFormatOf<uint>() { return DXGI_FORMAT_R32_UINT; }
    template<> consteval DXGI_FORMAT bufferFormatOf<int>() { return DXGI_FORMAT_R32_SINT; }
    template<> consteval DXGI_FORMAT bufferFormatOf<uint16>() { return DXGI_FORMAT_R16_UINT; }
    template<> consteval DXGI_FORMAT bufferFormatOf<int16>() { return DXGI_FORMAT_R16_SINT; }
}

template<>
struct fmt::formatter<D3D12_RESOURCE_STATES> {
    constexpr auto parse(fmt::format_parse_context& ctx) const { return ctx.begin(); }

    constexpr auto format(D3D12_RESOURCE_STATES states, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", sm::render::format(states));
    }
};
