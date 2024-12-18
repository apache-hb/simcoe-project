struct Vertex {
    float2 position : POSITION;
    float2 uv : TEXCOORD;
};

struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

Input vsMain(Vertex vtx) {
    Input result = { float4(vtx.position, 0.f, 1.f), vtx.uv };
    return result;
}

float4 psMain(Input input) : SV_TARGET {
    return gTexture.Sample(gSampler, input.uv);
}
