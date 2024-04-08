#include "common.hlsli"

Texture2D gTextures[] : register(t0, space1);
SamplerState gSampler : register(s0);

StructuredBuffer<LightVolumeData> gPointLightVolumes : register(t0);
StructuredBuffer<LightVolumeData> gSpotLightVolumes : register(t1);
StructuredBuffer<PointLightData> gPointLightParams : register(t2);
StructuredBuffer<SpotLightData> gSpotLightParams : register(t3);
Buffer<uint> gTileIndexBuffer : register(t4);

struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

// vertex shader for depth prepass

struct DepthPassVertexOutput {
    float4 position : SV_POSITION;
};

DepthPassVertexOutput vs_depth_pass(VertexInput input) {
    DepthPassVertexOutput output;
    output.position = mul(float4(input.position, 1.0f), gObjectData.worldViewProjection);
    return output;
}

// vertex shader for depth prepass with alpha testing

struct DepthPassAlphaTestVertexOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

DepthPassAlphaTestVertexOutput vs_depth_pass_alpha_test(VertexInput input) {
    DepthPassAlphaTestVertexOutput output;
    output.position = mul(float4(input.position, 1.0f), gObjectData.worldViewProjection);
    output.uv = input.uv;
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
    output.position = mul(float4(input.position, 1.0f), gObjectData.worldViewProjection);

    // these are all in world space
    output.worldPosition = mul(input.position, (float3x3)gObjectData.world);
    output.normal = mul(input.normal, (float3x3)gObjectData.world);
    output.tangent = mul(input.tangent, (float3x3)gObjectData.world);

    output.uv = input.uv;

    return output;
}

float4 ps_opaque(VertexOutput vin) : SV_TARGET {
    // TODO: implement all this

    return float4(vin.worldPosition.xyz, 1.0f);
}
