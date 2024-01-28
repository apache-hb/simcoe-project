struct Input {
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
    float2 uv : TEXCOORD;
};

cbuffer CanvasProjection : register(b0) {
    float4x4 projection;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 project(float2 pos) {
    return mul(projection, float4(pos, 0.f, 1.f));
}

Input vs_main(float2 position : POSITION, float2 uv : TEXCOORD, float4 colour : COLOUR) {
    Input result = { project(position), colour, uv };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    return input.colour * gTexture.Sample(gSampler, input.uv);
}
