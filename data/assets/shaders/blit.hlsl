struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

Input vs_main(float2 position : POSITION, float2 uv : TEXCOORD) {
    Input result = { float4(position, 0.f, 1.f), uv };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    return gTexture.Sample(gSampler, input.uv);
}
