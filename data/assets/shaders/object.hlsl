struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer CameraBuffer : register(b0) {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 camera(float3 position) {
    float4 result = mul(float4(position, 1.f), model);
    result = mul(result, view);
    result = mul(result, projection);
    return result;
}

Input vs_main(float3 position : POSITION, float2 uv : TEXCOORD) {
    Input result = { camera(position), uv };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    return gTexture.Sample(gSampler, input.uv);
}
