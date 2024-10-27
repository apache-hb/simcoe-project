#include "draw/common/opaque.h"

struct Vertex {
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer CameraBuffer : register(b0) {
    CameraInfo info;
};

float4 camera(float3 position) {
    return mul(float4(position, 1.f), info.mvp);
}

Input vsMain(Vertex vtx) {
    Input result = { camera(vtx.position), vtx.uv };
    return result;
}

float4 psMain(Input input) : SV_TARGET {
    return float4(input.uv, 1, 1);
}
