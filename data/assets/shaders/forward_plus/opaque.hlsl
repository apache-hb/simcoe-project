#include "common.hlsli"

// per object data
cbuffer ObjectBuffer : register(b1) {
    ObjectData gObjectData;
};

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

float4x4 getModelMatrix() {
    return gObjectData.model;
}

float4 project(float3 position) {
    float4x4 m = getModelMatrix();
    float4x4 v = gCameraData.worldView;
    float4x4 p = gCameraData.projection;

    float4x4 mvp = mul(mul(m, v), p);

    return mul(float4(position, 1.f), mvp);
}

// vertex shader for depth prepass

struct DepthPassVertexOutput {
    float4 position : SV_POSITION;
};

DepthPassVertexOutput vsDepthPass(float3 position : POSITION) {
    DepthPassVertexOutput output;
    output.position = project(position);
    return output;
}

// vertex shader for depth prepass with alpha testing

struct DepthPassAlphaTestVertexOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

DepthPassAlphaTestVertexOutput vsDepthPassAlphaTest(float3 position : POSITION, float2 uv : TEXCOORD) {
    DepthPassAlphaTestVertexOutput output;
    output.position = project(position);
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

VertexOutput vsOpaque(VertexInput input) {
    VertexOutput output;
    output.position = project(input.position);

    // these are all in world space
    float3x3 world = (float3x3)getModelMatrix();
    output.worldPosition = mul(input.position, world);
    output.normal = mul(input.normal, world);
    output.tangent = mul(input.tangent, world);

    output.uv = input.uv;

    return output;
}

float4 psOpaque(VertexOutput vin) : SV_TARGET {
    // TODO: implement all this

    return float4(vin.worldPosition.xyz, 1.0f);
}
