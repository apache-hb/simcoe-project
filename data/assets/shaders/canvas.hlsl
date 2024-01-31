struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 colour : COLOUR;
};

cbuffer CanvasProjection : register(b0) {
    float4x4 projection;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

Input vs_main(float2 position : POSITION, float2 uv : TEXCOORD, float4 colour : COLOUR) {
    Input result = { mul(projection, float4(position, 0.f, 1.f)), uv, colour };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    return input.colour * gTexture.Sample(gSampler, input.uv);
}
