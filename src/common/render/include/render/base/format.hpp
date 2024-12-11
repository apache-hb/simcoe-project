#pragma once

#include <directx/d3d12.h>

#include <fmtlib/format.h>

namespace sm::render {
    std::string format(D3D12_RESOURCE_STATES states);

    template<typename T>
    consteval DXGI_FORMAT bufferFormatOf() { return DXGI_FORMAT_UNKNOWN; }

    template<> consteval DXGI_FORMAT bufferFormatOf<float>() { return DXGI_FORMAT_R32_FLOAT; }
    template<> consteval DXGI_FORMAT bufferFormatOf<uint>() { return DXGI_FORMAT_R32_UINT; }
    template<> consteval DXGI_FORMAT bufferFormatOf<int>() { return DXGI_FORMAT_R32_SINT; }
    template<> consteval DXGI_FORMAT bufferFormatOf<uint16_t>() { return DXGI_FORMAT_R16_UINT; }
    template<> consteval DXGI_FORMAT bufferFormatOf<int16_t>() { return DXGI_FORMAT_R16_SINT; }

    constexpr DXGI_FORMAT getDepthTextureFormat(DXGI_FORMAT depth) {
        switch (depth) {
        case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24G8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32G8X24_TYPELESS;
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }

    constexpr DXGI_FORMAT getShaderViewFormat(DXGI_FORMAT depth) {
        switch (depth) {
        case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_UINT;
        case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_UINT;
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }
}

template<>
struct fmt::formatter<D3D12_RESOURCE_STATES> {
    constexpr auto parse(fmt::format_parse_context& ctx) const { return ctx.begin(); }

    constexpr auto format(D3D12_RESOURCE_STATES states, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", sm::render::format(states));
    }
};
