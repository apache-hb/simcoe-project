struct Vertex {
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer GlobalBuffer : register(b0) {
    float4x4 mvp;
    uint gAlbedoTextureIndex;
};

Texture2D gTextures[] : register(t0);
SamplerState gSampler : register(s0);

float4 camera(float3 position) {
    return mul(float4(position, 1.f), mvp);
}

Input vs_main(Vertex vtx) {
    Input result = { camera(vtx.position), vtx.uv };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    if (gAlbedoTextureIndex != 0xFFFFFFFF) {
        return gTextures[gAlbedoTextureIndex].Sample(gSampler, input.uv);
    }

    return float4(input.uv, 0, 1);
}
