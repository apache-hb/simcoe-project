#include "common.hpp"

struct Vertex {
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer ViewportBuffer : register(b0) {
    ViewportData gViewportData;
};

cbuffer ObjectBuffer : register(b1) {
    ObjectData gObjectData;
};

float4 project(float3 position) {
    float4x4 m = gObjectData.model;
    float4x4 v = gViewportData.worldView;
    float4x4 p = gViewportData.projection;

    float4x4 mvp = mul(mul(m, v), p);

    return mul(float4(position, 1.f), mvp);
}

Input vertexMain(Vertex vtx) {
    Input result = { project(vtx.position), vtx.uv };
    return result;
}

float4 pixelMain(Input input) : SV_TARGET {
    return float4(input.uv, 0, 1);
}
