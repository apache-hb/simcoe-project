#include "common.hlsli"

cbuffer ObjectBuffer : register(b1) {
    uint gObjectIndex;
};

// per object data
StructuredBuffer<ObjectData> gObjectData : register(t0);

Texture2D gTextures[] : register(t0, space1);
SamplerState gSampler : register(s0);

StructuredBuffer<LightVolumeData> gPointLightData : register(t1);
StructuredBuffer<LightVolumeData> gSpotLightData : register(t2);
StructuredBuffer<PointLightData> gPointLightParams : register(t3);
StructuredBuffer<SpotLightData> gSpotLightParams : register(t4);

Buffer<uint> gLightIndexBuffer : register(t5);

struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

// vertex shader for depth prepass

cbuffer ObjectBuffer : register(b0) {
    float4x4 gWorldViewProjection;
}

struct DepthPassVertexOutput {
    float4 position : SV_POSITION;
};

DepthPassVertexOutput vs_depth_pass(float3 position : POSITION) {
    DepthPassVertexOutput output;
    output.position = mul(float4(position, 1.0f), gWorldViewProjection);
    return output;
}

// vertex shader for depth prepass with alpha testing

struct DepthPassAlphaTestVertexOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

DepthPassAlphaTestVertexOutput vs_depth_pass_alpha_test(float3 position : POSITION, float2 uv : TEXCOORD) {
    DepthPassAlphaTestVertexOutput output;
    output.position = mul(float4(position, 1.0f), gObjectData[gObjectIndex].worldViewProjection);
    output.uv = uv;
    return output;
}

// vertex shader for opaque objects

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 worldPosition : TEXCOORD2;
};

VertexOutput vs_opaque(VertexInput input) {
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0f), gObjectData[gObjectIndex].worldViewProjection);

    // these are all in world space
    float3x3 world = (float3x3)gObjectData[gObjectIndex].world;
    output.worldPosition = mul(input.position, world);
    output.normal = mul(input.normal, world);
    output.tangent = mul(input.tangent, world);

    output.uv = input.uv;

    return output;
}

float4 ps_opaque(VertexOutput vin) : SV_TARGET {
    // TODO: implement all this

    return float4(vin.worldPosition.xyz, 1.0f);
}
