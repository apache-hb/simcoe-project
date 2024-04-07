// only the data needed for binning/culling
struct LightVolumeData {
    float3 position;
    float radius;
};

// data needed for shading point lights
struct PointLight {
    float3 colour;
};

// data needed for shading spot lights
struct SpotLight {
    float3 direction;
    float3 colour;
    float angle;
};

struct Material {
    float3 albedo;
    uint albedo_texture;

    float metallic;
    uint metallic_texture;

    float specular;
    uint specular_texture;

    float roughness;
    uint roughness_texture;

    uint normal_texture;
};
