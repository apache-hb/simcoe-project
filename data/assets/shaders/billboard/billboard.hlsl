Texture2DArray gTreeMapArray : register(t0);
SamplerState gSamplerState : register(s0);

struct VertexOut {
    float3 center : POSITION;
    float2 size : SIZE;
};

struct GeoOut {
    float4 positionH : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float2 uv : TEXCOORD;
    uint prim : SV_PRIMITIVEID;
};

cbuffer ObjectBuffer : register(b0) {
    uint gTreeMapLength;
};

cbuffer CameraBuffer : register(b1) {
    float4x4 gViewProjection;
    float3 gCameraPosition;
};

static float2 kQuadUV[] = {
    float2(0, 1),
    float2(0, 0),
    float2(1, 1),
    float2(1, 0)
};

VertexOut vsBillBoard(float3 position : POSITION, float2 size : SIZE) {
    VertexOut result;
    result.center = position;
    result.size = size;
    return result;
}

[maxvertexcount(4)]
void gsBillBoard(
    point VertexOut vout[1],
    uint prim : SV_PrimitiveID,
    inout TriangleStream<GeoOut> stream)
{
    VertexOut gin = vout[0];

    float3 up = float3(0, 0, 1);
    float3 look = gCameraPosition - gin.center;
    look.y = 0;
    look = normalize(look);
    float3 right = cross(up, look);

    float2 halfSize = 0.5f * gin.size;

    float4 p0 = float4(gin.center + halfSize.x * right + halfSize.y * up, 1);
    float4 p1 = float4(gin.center - halfSize.x * right + halfSize.y * up, 1);
    float4 p2 = float4(gin.center + halfSize.x * right - halfSize.y * up, 1);
    float4 p3 = float4(gin.center - halfSize.x * right - halfSize.y * up, 1);

    float4 p[4] = { p0, p1, p2, p3 };

    [unroll]
    for (int i = 0; i < 4; i++) {
        GeoOut gout;
        gout.positionH = mul(p[i], gViewProjection);
        gout.positionW = p[i].xyz;
        gout.normalW = look;
        gout.uv = kQuadUV[i];
        gout.prim = prim;

        stream.Append(gout);
    }
}

// float4 psBillBoard(GeoOut pin) : SV_TARGET {
//     float3 uvw = float3(pin.uv, pin.prim % gTreeMapLength);
//     float4 albedo = gTreeMapArray.Sample(gSamplerState, uvw);
//     float3 normalW = normalize(pin.normalW);

//     float3 toCameraW = gCameraPosition - pin.positionW;
//     float distanceToCamera = length(toCameraW);
//     toCameraW /= distanceToCamera;
// }
