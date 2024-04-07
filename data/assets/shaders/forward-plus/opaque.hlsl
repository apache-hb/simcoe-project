#include "common.hlsli"

Texture2D gTextures[] : register(t0, space1);
SamplerState gSampler : register(s0);

StructuredBuffer<LightVolumeData> gPointLightVolumes : register(t0);
StructuredBuffer<LightVolumeData> gSpotLightVolumes : register(t1);
StructuredBuffer<PointLight> gPointLightParams : register(t2);
StructuredBuffer<SpotLight> gSpotLightParams : register(t3);
Buffer<uint> gTileIndexBuffer : register(t4);
