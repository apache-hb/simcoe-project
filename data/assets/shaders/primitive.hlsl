#include "common.hlsli"

struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer CameraBuffer : register(b0) {
    float4x4 mvp;
};

// Texture2D gTextures : register(t0);
// SamplerState gSampler : register(s0);

// StructuredBuffer<Material> gMaterials : register(t1);
// StructuredBuffer<PointLight> gPointLights : register(t2);

float4 camera(float3 position) {
    return mul(float4(position, 1.f), mvp);
}

Input vs_main(float3 position : POSITION, float2 uv : TEXCOORD) {
    Input result = { camera(position), uv };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    return float4(input.uv, 0, 1);
}
