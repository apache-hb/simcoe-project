struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer CameraBuffer : register(b0) {
    float4x4 mvp;
};

cbuffer MaterialBuffer : register(b1) {
    uint albedo_idx;
    uint normal_idx;
    uint metallic_idx;
    uint roughness_idx;
    uint ao_idx;
};

Texture2D gTextures[] : register(t0);
SamplerState gSampler : register(s0);

float4 camera(float3 position) {
    return mul(float4(position, 1.f), mvp);
}

Input vs_main(float3 position : POSITION, float2 uv : TEXCOORD) {
    Input result = { camera(position), uv };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    float4 albedo = gTextures[albedo_idx].Sample(gSampler, input.uv);

    // TODO: implement pbr

    return albedo;
}
