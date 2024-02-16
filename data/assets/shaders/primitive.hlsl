struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer CameraBuffer : register(b0) {
    float4x4 mvp;
};

float4 camera(float3 position) {
    return mul(float4(position, 1.f), mvp);
}

Input vs_main(float3 position : POSITION, float2 uv : TEXCOORD) {
    Input result = { camera(position), uv };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    return float4(input.uv, 0.f, 1.f);
}
