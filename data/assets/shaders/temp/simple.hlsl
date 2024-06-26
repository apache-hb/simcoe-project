struct Input {
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

Input vs_main(float4 position : POSITION, float4 colour : COLOUR) {
    Input result = { position, colour };
    return result;
}

float4 ps_main(Input input) : SV_TARGET {
    return input.colour;
}
