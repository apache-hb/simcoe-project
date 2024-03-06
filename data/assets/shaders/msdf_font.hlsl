struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer TextConstants : register(b0) {
    float4 fg_colour;
    float4 bg_colour;
    uint screen_px_range;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

Input vs_main(float2 position : POSITION, float2 uv : TEXCOORD) {
    Input result = { float4(position, 0.f, 1.f), uv };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    float3 msd = gTexture.Sample(gSampler, input.uv).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screen_px_distance = screen_px_range * (sd - 0.5);
    float opacity = clamp(screen_px_distance + 0.5, 0, 1);

    return lerp(bg_colour, fg_colour, opacity);
}
