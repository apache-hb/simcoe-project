// SPDX-license-Identifier: MIT

#ifndef __D3DX12_FORMAT_H__
#define __D3DX12_FORMAT_H__

#if defined( __cplusplus ) && __cpp_constexpr >= 201304L
  #define D3DX12_CONSTEXPR constexpr
#else
  #define D3DX12_CONSTEXPR inline
#endif

#include "d3d12.h"

#if defined( __clang__ )
  // use _Pragma rather than #pragma to satisfy older clang versions
  _Pragma("clang diagnostic push")
  _Pragma("clang diagnostic ignored \"-Wswitch\"")
#elif defined( __GNUC__ )
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wswitch"
#elif defined( _MSC_VER )
  #pragma warning(push)
  #pragma warning(disable:4061) // enumerator 'identifier' in a switch of enum 'enumeration' is not explicitly handled by a case label
  #pragma warning(disable:4062) // enumerator 'identifier' in a switch of enum 'enumeration' is not handled
#endif

#define D3DX12_EXACT_CASE( value ) case value: return #value

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_COMMAND_LIST_TYPE value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_COMMAND_LIST_TYPE_DIRECT);
    D3DX12_EXACT_CASE(D3D12_COMMAND_LIST_TYPE_BUNDLE);
    D3DX12_EXACT_CASE(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    D3DX12_EXACT_CASE(D3D12_COMMAND_LIST_TYPE_COPY);
    D3DX12_EXACT_CASE(D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE);
    D3DX12_EXACT_CASE(D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS);
    D3DX12_EXACT_CASE(D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_COMMAND_QUEUE_FLAGS value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_COMMAND_QUEUE_FLAG_NONE);
    D3DX12_EXACT_CASE(D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_COMMAND_QUEUE_PRIORITY value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_COMMAND_QUEUE_PRIORITY_NORMAL);
    D3DX12_EXACT_CASE(D3D12_COMMAND_QUEUE_PRIORITY_HIGH);
    D3DX12_EXACT_CASE(D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_PRIMITIVE_TOPOLOGY_TYPE value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED);
    D3DX12_EXACT_CASE(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
    D3DX12_EXACT_CASE(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_INPUT_CLASSIFICATION value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
    D3DX12_EXACT_CASE(D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_FILL_MODE value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_FILL_MODE_WIREFRAME);
    D3DX12_EXACT_CASE(D3D12_FILL_MODE_SOLID);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_CULL_MODE value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_CULL_MODE_NONE);
    D3DX12_EXACT_CASE(D3D12_CULL_MODE_FRONT);
    D3DX12_EXACT_CASE(D3D12_CULL_MODE_BACK);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_COMPARISON_FUNC value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_COMPARISON_FUNC_NEVER);
    D3DX12_EXACT_CASE(D3D12_COMPARISON_FUNC_LESS);
    D3DX12_EXACT_CASE(D3D12_COMPARISON_FUNC_EQUAL);
    D3DX12_EXACT_CASE(D3D12_COMPARISON_FUNC_LESS_EQUAL);
    D3DX12_EXACT_CASE(D3D12_COMPARISON_FUNC_GREATER);
    D3DX12_EXACT_CASE(D3D12_COMPARISON_FUNC_NOT_EQUAL);
    D3DX12_EXACT_CASE(D3D12_COMPARISON_FUNC_GREATER_EQUAL);
    D3DX12_EXACT_CASE(D3D12_COMPARISON_FUNC_ALWAYS);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_DEPTH_WRITE_MASK value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_DEPTH_WRITE_MASK_ZERO);
    D3DX12_EXACT_CASE(D3D12_DEPTH_WRITE_MASK_ALL);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_STENCIL_OP value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_STENCIL_OP_KEEP);
    D3DX12_EXACT_CASE(D3D12_STENCIL_OP_ZERO);
    D3DX12_EXACT_CASE(D3D12_STENCIL_OP_REPLACE);
    D3DX12_EXACT_CASE(D3D12_STENCIL_OP_INCR_SAT);
    D3DX12_EXACT_CASE(D3D12_STENCIL_OP_DECR_SAT);
    D3DX12_EXACT_CASE(D3D12_STENCIL_OP_INVERT);
    D3DX12_EXACT_CASE(D3D12_STENCIL_OP_INCR);
    D3DX12_EXACT_CASE(D3D12_STENCIL_OP_DECR);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_BLEND value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_BLEND_ZERO);
    D3DX12_EXACT_CASE(D3D12_BLEND_ONE);
    D3DX12_EXACT_CASE(D3D12_BLEND_SRC_COLOR);
    D3DX12_EXACT_CASE(D3D12_BLEND_INV_SRC_COLOR);
    D3DX12_EXACT_CASE(D3D12_BLEND_SRC_ALPHA);
    D3DX12_EXACT_CASE(D3D12_BLEND_INV_SRC_ALPHA);
    D3DX12_EXACT_CASE(D3D12_BLEND_DEST_ALPHA);
    D3DX12_EXACT_CASE(D3D12_BLEND_INV_DEST_ALPHA);
    D3DX12_EXACT_CASE(D3D12_BLEND_DEST_COLOR);
    D3DX12_EXACT_CASE(D3D12_BLEND_INV_DEST_COLOR);
    D3DX12_EXACT_CASE(D3D12_BLEND_SRC_ALPHA_SAT);
    D3DX12_EXACT_CASE(D3D12_BLEND_BLEND_FACTOR);
    D3DX12_EXACT_CASE(D3D12_BLEND_INV_BLEND_FACTOR);
    D3DX12_EXACT_CASE(D3D12_BLEND_SRC1_COLOR);
    D3DX12_EXACT_CASE(D3D12_BLEND_INV_SRC1_COLOR);
    D3DX12_EXACT_CASE(D3D12_BLEND_SRC1_ALPHA);
    D3DX12_EXACT_CASE(D3D12_BLEND_INV_SRC1_ALPHA);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_BLEND_OP value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_BLEND_OP_ADD);
    D3DX12_EXACT_CASE(D3D12_BLEND_OP_SUBTRACT);
    D3DX12_EXACT_CASE(D3D12_BLEND_OP_REV_SUBTRACT);
    D3DX12_EXACT_CASE(D3D12_BLEND_OP_MIN);
    D3DX12_EXACT_CASE(D3D12_BLEND_OP_MAX);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_COLOR_WRITE_ENABLE value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_COLOR_WRITE_ENABLE_RED);
    D3DX12_EXACT_CASE(D3D12_COLOR_WRITE_ENABLE_GREEN);
    D3DX12_EXACT_CASE(D3D12_COLOR_WRITE_ENABLE_BLUE);
    D3DX12_EXACT_CASE(D3D12_COLOR_WRITE_ENABLE_ALPHA);
    D3DX12_EXACT_CASE(D3D12_COLOR_WRITE_ENABLE_ALL);
    case (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN):
        return "D3D12_COLOR_WRITE_ENABLE_(RED|GREEN)";
    case (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_BLUE):
        return "D3D12_COLOR_WRITE_ENABLE_(RED|BLUE)";
    case (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_ALPHA):
        return "D3D12_COLOR_WRITE_ENABLE_(RED|ALPHA)";
    case (D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE):
        return "D3D12_COLOR_WRITE_ENABLE_(GREEN|BLUE)";
    case (D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_ALPHA):
        return "D3D12_COLOR_WRITE_ENABLE_(GREEN|ALPHA)";
    case (D3D12_COLOR_WRITE_ENABLE_BLUE | D3D12_COLOR_WRITE_ENABLE_ALPHA):
        return "D3D12_COLOR_WRITE_ENABLE_(BLUE|ALPHA)";
    case (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE):
        return "D3D12_COLOR_WRITE_ENABLE_(RED|GREEN|BLUE)";
    case (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_ALPHA):
        return "D3D12_COLOR_WRITE_ENABLE_(RED|GREEN|ALPHA)";
    case (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_BLUE | D3D12_COLOR_WRITE_ENABLE_ALPHA):
        return "D3D12_COLOR_WRITE_ENABLE_(RED|BLUE|ALPHA)";
    case (D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE | D3D12_COLOR_WRITE_ENABLE_ALPHA):
        return "D3D12_COLOR_WRITE_ENABLE_(GREEN|BLUE|ALPHA)";
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_LOGIC_OP value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_CLEAR);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_SET);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_COPY);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_COPY_INVERTED);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_NOOP);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_INVERT);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_AND);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_NAND);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_OR);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_NOR);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_XOR);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_EQUIV);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_AND_REVERSE);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_AND_INVERTED);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_OR_REVERSE);
    D3DX12_EXACT_CASE(D3D12_LOGIC_OP_OR_INVERTED);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_CONSERVATIVE_RASTERIZATION_MODE value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
    D3DX12_EXACT_CASE(D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON);
    default:
        return "Unknown";
    }
}

D3DX12_CONSTEXPR const char *D3DX12FormatToStringExact(D3D12_SHADER_MIN_PRECISION_SUPPORT value)
{
    switch (value)
    {
    D3DX12_EXACT_CASE(D3D12_SHADER_MIN_PRECISION_SUPPORT_NONE);
    D3DX12_EXACT_CASE(D3D12_SHADER_MIN_PRECISION_SUPPORT_10_BIT);
    D3DX12_EXACT_CASE(D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT);
    default:
        return "Unknown";
    }
}

#undef D3DX12_EXACT_CASE

#if defined( __clang__ )
  _Pragma("clang diagnostic pop")
#elif defined( __GNUC__ )
  #pragma GCC diagnostic pop
#elif defined( _MSC_VER )
  #pragma warning(pop)
#endif

#endif // __D3DX12_FORMAT_H__
