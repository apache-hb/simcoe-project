struct Vertex {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer CameraBuffer : register(b0) {
    float4x4 mvp;
};

float4 camera(float3 position) {
    return mul(float4(position, 1.f), mvp);
}

Input vsMain(Vertex vtx) {
    Input result = { camera(vtx.position), vtx.uv };
    return result;
}

float4 psMain(Input input) : SV_TARGET {
    return float4(input.uv, 1, 1);
}
