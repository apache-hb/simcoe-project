struct Input {
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

cbuffer CameraBuffer : register(b0) {
    float4x4 mvp;
};

float4 camera(float3 position) {
    return mul(float4(position, 1.f), mvp);
}

Input vs_main(float3 position : POSITION, float4 colour : COLOUR) {
    Input result = { camera(position), colour };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    return input.colour;
}
