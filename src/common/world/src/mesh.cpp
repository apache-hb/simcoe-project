#include "stdafx.hpp"

#include "world/mesh.hpp"
#include "world/ecs.hpp"

using namespace sm;
using namespace sm::world;

static constexpr VertexFlags kPrimitiveVertexFlags = VertexFlags::eNone;

Transform world::default_transform() {
    Transform result = {
        .position = 0.f,
        .rotation = math::quatf::identity(),
        .scale = 1.f
    };

    return result;
}

math::float4x4 Transform::matrix() const {
    return math::float4x4::transform(position, rotation, scale);
}

Transform Transform::operator*(const Transform& other) const {
    Transform result = {
        .position = position + rotation * (scale * other.position),
        .rotation = rotation * other.rotation,
        .scale = scale * other.scale
    };

    return result;
}

VertexFlags world::primitiveVertexBufferFlags() {
    return kPrimitiveVertexFlags;
}

DXGI_FORMAT world::primitiveIndexBufferFormat() {
    return DXGI_FORMAT_R16_UINT;
}

math::float3 BoxBounds::getCenter() const {
    return (min + max) / 2.f;
}

math::float3 BoxBounds::getExtents() const {
    return (max - min) / 2.f;
}

template <>
struct std::hash<Vertex> {
    constexpr auto operator()(const Vertex &vertex) const {
        size_t hash = 0;
        sm::hash_combine(hash, vertex.position);
        sm::hash_combine(hash, vertex.normal);
        sm::hash_combine(hash, vertex.uv);
        sm::hash_combine(hash, vertex.tangent);
        return hash;
    }
};

struct MeshBuilder {
    // TODO: probably want to merge verticies that are nearly the same
    sm::HashMap<Vertex, uint16> lookup;
    world::VertexBuffer vertices;
    world::IndexBuffer indices;

    uint16 addVertex(const Vertex &vertex) {
        auto [it, inserted] = lookup.try_emplace(vertex, uint16(vertices.size()));
        if (inserted) {
            vertices.push_back(vertex);
        }
        return it->second;
    }

    void triangle(Vertex v0, Vertex v1, Vertex v2) {
        auto i0 = addVertex(v0);
        auto i1 = addVertex(v1);
        auto i2 = addVertex(v2);

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);
    }

    void quad(Vertex v0, Vertex v1, Vertex v2, Vertex v3) {
        auto i0 = addVertex(v0);
        auto i1 = addVertex(v1);
        auto i2 = addVertex(v2);
        auto i3 = addVertex(v3);

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        indices.push_back(i0);
        indices.push_back(i2);
        indices.push_back(i3);
    }

    Mesh build() {
        Mesh mesh = {
            .vertices = std::move(vertices),
            .indices = std::move(indices),
        };

        return mesh;
    }
};

static Vertex midpoint(const Vertex& v0, const Vertex& v1) {
	return {
		.position = (v0.position + v1.position) / 2.f,
        .normal = (v0.normal + v1.normal) / 2.f,
        .uv = (v0.uv + v1.uv) / 2.f,
        .tangent = (v0.tangent + v1.tangent) / 2.f
	};
}

static void subdivide(Mesh& mesh) {
    auto& [vertices, indices] = mesh;
    world::IndexBuffer oldIndices = indices.clone();
    world::VertexBuffer oldVertices = vertices.clone();

    indices.clear();
    vertices.clear();

    int count = int(oldIndices.size() / 3);
    for (int i = 0; i < count; i++) {
        Vertex v0 = oldVertices[oldIndices[i * 3 + 0]];
        Vertex v1 = oldVertices[oldIndices[i * 3 + 1]];
        Vertex v2 = oldVertices[oldIndices[i * 3 + 2]];

        Vertex m0 = midpoint(v0, v1);
        Vertex m1 = midpoint(v1, v2);
        Vertex m2 = midpoint(v0, v2);

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(m0);
        vertices.push_back(m1);
        vertices.push_back(m2);

        indices.push_back(i * 6 + 0);
        indices.push_back(i * 6 + 3);
        indices.push_back(i * 6 + 5);

        indices.push_back(i * 6 + 3);
        indices.push_back(i * 6 + 4);
        indices.push_back(i * 6 + 5);

        indices.push_back(i * 6 + 5);
        indices.push_back(i * 6 + 4);
        indices.push_back(i * 6 + 2);

        indices.push_back(i * 6 + 3);
        indices.push_back(i * 6 + 1);
        indices.push_back(i * 6 + 4);
    }
}

static Mesh cube(const world::Cube &cube) {
    auto [w, h, d] = cube;

    MeshBuilder builder;

    builder.quad(
        {.position = {-w, -h, -d}, .uv = {1, 1}},
        {.position = {-w, -h, d}, .uv = {1, 0}},
        {.position = {-w, h, d}, .uv = {0, 0}},
        {.position = {-w, h, -d}, .uv = {0, 1}}
    ); // back

    builder.quad(
        {.position = {w, h, -d}, .uv = {1, 1}},
        {.position = {w, h, d}, .uv = {1, 0}},
        {.position = {w, -h, d}, .uv = {0, 0}},
        {.position = {w, -h, -d}, .uv = {0, 1}}
    ); // front

    builder.quad(
        {.position = {w, -h, -d}, .uv = {1, 1}},
        {.position = {w, -h, d}, .uv = {1, 0}},
        {.position = {-w, -h, d}, .uv = {0, 0}},
        {.position = {-w, -h, -d}, .uv = {0, 1}}
    ); // left

    builder.quad(
        {.position = {-w, h, -d}, .uv = {1, 1}},
        {.position = {-w, h, d}, .uv = {1, 0}},
        {.position = {w, h, d}, .uv = {0, 0}},
        {.position = {w, h, -d}, .uv = {0, 1}}
    ); // right

    builder.quad(
        {.position = {-w, -h, -d}, .uv = {1, 1}},
        {.position = {-w, h, -d}, .uv = {1, 0}},
        {.position = {w, h, -d}, .uv = {0, 0}},
        {.position = {w, -h, -d}, .uv = {0, 1}}
    ); // bottom

    builder.quad(
        {.position = {w, -h, d}, .uv = {1, 1}},
        {.position = {w, h, d}, .uv = {1, 0}},
        {.position = {-w, h, d}, .uv = {0, 0}},
        {.position = {-w, -h, d}, .uv = {0, 1}}
    ); // top

    return builder.build();
}

// yoink: https://github.com/microsoft/DirectXTK12/blob/main/Src/Geometry.cpp#L147
static Mesh sphere(const world::Sphere& sphere) {
    auto [radius, slices, stacks] = sphere;
    MeshBuilder builder;

    int horizontalSegments = slices;
    int verticalSegments = stacks;

    // Create rings of vertices at progressively higher latitudes.
    for (int i = 0; i <= verticalSegments; i++) {
        const float v = 1 - float(i) / float(verticalSegments);

        const float latitude = (float(i) * math::pi() / float(verticalSegments)) - math::pidiv2();
        auto [dy, dxz] = math::sincos(latitude);

        // Create a single ring of vertices at this latitude.
        for (int j = 0; j <= horizontalSegments; j++) {
            const float u = float(j) / float(horizontalSegments);

            const float longitude = float(j) * math::pi2() / float(horizontalSegments);
            auto [dx, dz] = math::sincos(longitude);

            dx *= dxz;
            dz *= dxz;

            math::float3 normal = {dx, dy, dz};
            math::float2 uv = {u, v};
            math::float3 position = normal * radius;

            builder.vertices.push_back({ position, normal, uv });
        }
    }

    // Fill the index buffer with triangles joining each pair of latitude rings.
    const uint16 stride = horizontalSegments + 1;

    for (int i = 0; i < verticalSegments; i++) {
        for (int j = 0; j <= horizontalSegments; j++) {
            const uint16 nextI = i + 1;
            const uint16 nextJ = (j + 1) % stride;

            builder.indices.push_back(i * stride + j);
            builder.indices.push_back(nextI * stride + j);
            builder.indices.push_back(i * stride + nextJ);

            builder.indices.push_back(i * stride + nextJ);
            builder.indices.push_back(nextI * stride + j);
            builder.indices.push_back(nextI * stride + nextJ);
        }
    }


    return builder.build();
}

static Mesh cylinder(const world::Cylinder& cylinder) {
    auto [radius, height, slices] = cylinder;
    MeshBuilder builder;

    for (int i = 0; i < slices; ++i) {
        float theta0 = float(i) * 2 * 3.14f / float(slices);
        float theta1 = float(i + 1) * 2 * 3.14f / float(slices);
        auto [s0, c0] = math::sincos(theta0);
        auto [s1, c1] = math::sincos(theta1);

        // make top half of the cylinder
        {
            Vertex v0 = {
                .position = math::float3(radius * c0, height / 2, radius * s0),
                .uv = math::float2(float(i) / float(slices), 0)
            };
            Vertex v1 = {
                .position = math::float3(radius * c1, height / 2, radius * s1),
                .uv = math::float2(float(i + 1) / float(slices), 0)
            };
            Vertex v2 = {
                .position = math::float3(0, height / 2, 0),
                .uv = math::float2(0.5f, 0)
            };

            builder.triangle(v0, v2, v1);
        }

        // make bottom half of the cylinder
        {
            Vertex v0 = {
                .position = math::float3(radius * c0, -height / 2, radius * s0),
                .uv = math::float2(float(i) / float(slices), 1)
            };
            Vertex v1 = {
                .position = math::float3(radius * c1, -height / 2, radius * s1),
                .uv = math::float2(float(i + 1) / float(slices), 1)
            };
            Vertex v2 = {
                .position = math::float3(0, -height / 2, 0),
                .uv = math::float2(0.5f, 1)
            };

            builder.triangle(v0, v1, v2);
        }

        // make body
        {
            Vertex v0 = {
                .position = math::float3(radius * c0, -height / 2, radius * s0),
                .uv = math::float2(float(i) / float(slices), 0)
            };
            Vertex v1 = {
                .position = math::float3(radius * c1, -height / 2, radius * s1),
                .uv = math::float2(float(i + 1) / float(slices), 0)
            };
            Vertex v2 = {
                .position = math::float3(radius * c0, height / 2, radius * s0),
                .uv = math::float2(float(i) / float(slices), 1)
            };
            Vertex v3 = {
                .position = math::float3(radius * c1, height / 2, radius * s1),
                .uv = math::float2(float(i + 1) / float(slices), 1)
            };

            builder.quad(v2, v3, v1, v0);
        }
    }

    return builder.build();
}

static Mesh plane(const world::Plane& plane) {
    auto [width, depth] = plane;
    MeshBuilder builder;

    // positions
    math::float3 p0 = { -width / 2, 0, -depth / 2 };
    math::float3 p1 = { width / 2, 0, -depth / 2 };
    math::float3 p2 = { width / 2, 0, depth / 2 };
    math::float3 p3 = { -width / 2, 0, depth / 2 };

    // normal
    math::float3 n = { 0, 1, 0 };

    // uv coordinates
    math::float2 uv0 = { 0, 0 };
    math::float2 uv1 = { 1, 0 };
    math::float2 uv2 = { 1, 1 };
    math::float2 uv3 = { 0, 1 };

    // tangent
    math::float3 t = { 1, 0, 0 };

    Vertex v0 = { p0, n, uv0, t };
    Vertex v1 = { p1, n, uv1, t };
    Vertex v2 = { p2, n, uv2, t };
    Vertex v3 = { p3, n, uv3, t };

    builder.quad(v0, v1, v2, v3);

    return builder.build();
}

static Mesh wedge(const world::Wedge& wedge) {
    auto [width, height, depth] = wedge;
    MeshBuilder builder;

    float w2 = width / 2;
    float h2 = height / 2;
    float d2 = depth / 2;

    // back quad
    {
        Vertex v0 = { math::float3(-w2, -h2, d2), };
        Vertex v1 = { math::float3(w2, -h2, d2), };
        Vertex v2 = { math::float3(w2, -h2, -d2), };
        Vertex v3 = { math::float3(-w2, -h2, -d2), };

        builder.quad(v0, v1, v2, v3);
    }

    // bottom quad
    {
        Vertex v0 = { math::float3(-w2, -h2, d2), };
        Vertex v1 = { math::float3(w2, -h2, d2), };
        Vertex v2 = { math::float3(w2, h2, -d2), };
        Vertex v3 = { math::float3(-w2, h2, -d2), };

        builder.quad(v3, v2, v1, v0);
    }

    // left tri
    {
        Vertex v0 = { math::float3(-w2, -h2, d2), };
        Vertex v1 = { math::float3(-w2, -h2, -d2), };
        Vertex v2 = { math::float3(-w2, h2, -d2), };

        builder.triangle(v0, v1, v2);
    }

    // right tri
    {
        Vertex v0 = { math::float3(w2, -h2, d2), };
        Vertex v1 = { math::float3(w2, h2, -d2), };
        Vertex v2 = { math::float3(w2, -h2, -d2), };

        builder.triangle(v0, v1, v2);
    }

    // front quad
    {
        Vertex v0 = { math::float3(-w2, -h2, -d2), };
        Vertex v1 = { math::float3(w2, -h2, -d2), };
        Vertex v2 = { math::float3(w2, h2, -d2), };
        Vertex v3 = { math::float3(-w2, h2, -d2), };

        builder.quad(v0, v1, v2, v3);
    }

    return builder.build();
}

static Mesh capsule(const world::Capsule& capsule) {
    MeshBuilder builder;
    auto [radius, height, slices, rings] = capsule;

    for (int i = 0; i < slices; i++) {
        float theta0 = float(i) * 2 * 3.14f / float(slices);
        float theta1 = float(i + 1) * 2 * 3.14f / float(slices);
        auto [s0, c0] = math::sincos(theta0);
        auto [s1, c1] = math::sincos(theta1);

        for (int j = 0; j < rings; j++) {
            float phi0 = float(j) * 3.14f / float(rings);
            float phi1 = float(j + 1) * 3.14f / float(rings);
            auto [sp0, cp0] = math::sincos(phi0);
            auto [sp1, cp1] = math::sincos(phi1);

            // make top cap of the capsule
            {
                Vertex v0 = { math::float3(radius * cp0 * c0, height / 2 + radius * sp0, radius * cp0 * s0) };
                Vertex v1 = { math::float3(radius * cp1 * c0, height / 2 + radius * sp1, radius * cp1 * s0) };
                Vertex v2 = { math::float3(radius * cp0 * c1, height / 2 + radius * sp0, radius * cp0 * s1) };
                Vertex v3 = { math::float3(radius * cp1 * c1, height / 2 + radius * sp1, radius * cp1 * s1) };

                builder.quad(v0, v1, v3, v2);
            }

            // make bottom cap of the capsule
            {
                Vertex v0 = { math::float3(radius * cp0 * c0, -height / 2 - radius * sp0, radius * cp0 * s0) };
                Vertex v1 = { math::float3(radius * cp1 * c0, -height / 2 - radius * sp1, radius * cp1 * s0) };
                Vertex v2 = { math::float3(radius * cp0 * c1, -height / 2 - radius * sp0, radius * cp0 * s1) };
                Vertex v3 = { math::float3(radius * cp1 * c1, -height / 2 - radius * sp1, radius * cp1 * s1) };

                builder.quad(v0, v2, v3, v1);
            }
        }

        // make body
        {
            Vertex v0 = { math::float3(radius * c0, -height / 2, radius * s0) };
            Vertex v1 = { math::float3(radius * c1, -height / 2, radius * s1) };
            Vertex v2 = { math::float3(radius * c0, height / 2, radius * s0) };
            Vertex v3 = { math::float3(radius * c1, height / 2, radius * s1) };

            builder.quad(v0, v1, v3, v2);
        }
    }

    return builder.build();
}

static Mesh diamond(const world::Diamond& diamond) {
    MeshBuilder builder;
    auto [width, height, depth] = diamond;
	float w2 = width / 2;
	float h2 = height / 2;
	float d2 = depth / 2;

	// right side

	{
		Vertex v2 = { math::float3(0, 0, -d2), };
		Vertex v1 = { math::float3(-w2, 0, 0), };
		Vertex v0 = { math::float3(0, h2, 0), };

		builder.triangle(v0, v1, v2);
	}

	{
		Vertex v0 = { math::float3(0, 0, -d2), };
		Vertex v2 = { math::float3(0, h2, 0), };
		Vertex v1 = { math::float3(w2, 0, 0), };

		builder.triangle(v0, v1, v2);
	}

	{
		Vertex v0 = { math::float3(0, 0, -d2), };
		Vertex v2 = { math::float3(-w2, 0, 0), };
		Vertex v1 = { math::float3(0, -h2, 0), };

		builder.triangle(v0, v2, v1);
	}

	{
		Vertex v0 = { math::float3(0, 0, -d2), };
		Vertex v2 = { math::float3(0, -h2, 0), };
		Vertex v1 = { math::float3(w2, 0, 0), };

		builder.triangle(v0, v2, v1);
	}

	// left side

	{
		Vertex v2 = { math::float3(0, 0, d2), };
		Vertex v0 = { math::float3(-w2, 0, 0), };
		Vertex v1 = { math::float3(0, h2, 0), };

		builder.triangle(v0, v1, v2);
	}

	{
		Vertex v0 = { math::float3(0, 0, d2), };
		Vertex v1 = { math::float3(0, h2, 0), };
		Vertex v2 = { math::float3(w2, 0, 0), };

		builder.triangle(v0, v1, v2);
	}

	{
		Vertex v0 = { math::float3(0, 0, d2), };
		Vertex v1 = { math::float3(-w2, 0, 0), };
		Vertex v2 = { math::float3(0, -h2, 0), };

		builder.triangle(v0, v2, v1);
	}

	{
		Vertex v0 = { math::float3(0, 0, d2), };
		Vertex v1 = { math::float3(0, -h2, 0), };
		Vertex v2 = { math::float3(w2, 0, 0), };

		builder.triangle(v0, v2, v1);
	}

    return builder.build();
}

static Mesh geosphere(const world::GeoSphere& geosphere) {
    auto [radius, tessellation] = geosphere;
    MeshBuilder builder;

    // Create a icosahedron
    const float x = 0.525731f;
    const float z = 0.850651f;

    const world::Vertex vertices[] = {
        { math::float3(-x, 0, z)  * radius },
        { math::float3(x, 0, z)   * radius },
        { math::float3(-x, 0, -z) * radius },
        { math::float3(x, 0, -z)  * radius },
        { math::float3(0, z, x)   * radius },
        { math::float3(0, z, -x)  * radius },
        { math::float3(0, -z, x)  * radius },
        { math::float3(0, -z, -x) * radius },
        { math::float3(z, x, 0)   * radius },
        { math::float3(-z, x, 0)  * radius },
        { math::float3(z, -x, 0)  * radius },
        { math::float3(-z, -x, 0) * radius },
    };

	static constexpr uint16 kIndices[60] = {
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	builder.vertices = vertices;
	builder.indices = kIndices;

	auto mesh = builder.build();

	for (int i = 0; i < tessellation; i++)
		subdivide(mesh);

	for (auto& v : mesh.vertices) {
        math::float3 normal = v.position.normalized();
		v.position = normal * radius;
		// v.normal = normal;
	}

	// redo the vertex winding
	for (size_t i = 0; i < mesh.indices.size(); i += 3)
		std::swap(mesh.indices[i], mesh.indices[i + 2]);

    return mesh;
}

Mesh world::primitive(const Cube& cube) {
    return ::cube(cube);
}

Mesh world::primitive(const Sphere& sphere) {
    return ::sphere(sphere);
}

Mesh world::primitive(const Cylinder& cylinder) {
    return ::cylinder(cylinder);
}

Mesh world::primitive(const Plane& plane) {
    return ::plane(plane);
}

Mesh world::primitive(const Wedge& wedge) {
    return ::wedge(wedge);
}

Mesh world::primitive(const Capsule& capsule) {
    return ::capsule(capsule);
}

Mesh world::primitive(const Diamond& diamond) {
    return ::diamond(diamond);
}

Mesh world::primitive(const GeoSphere& geosphere) {
    return ::geosphere(geosphere);
}

/// calculating aabbs for primitives

world::ecs::AABB world::ecs::bounds(world::Cube cube) {
    math::float3 min = {-cube.width / 2, -cube.height / 2, -cube.depth / 2};
    math::float3 max = {cube.width / 2, cube.height / 2, cube.depth / 2};

    return {min, max};
}

world::ecs::AABB world::ecs::bounds(world::Sphere sphere) {
    math::float3 min = {-sphere.radius, -sphere.radius, -sphere.radius};
    math::float3 max = {sphere.radius, sphere.radius, sphere.radius};

    return {min, max};
}

world::ecs::AABB world::ecs::bounds(world::Cylinder cylinder) {
    math::float3 min = {-cylinder.radius, -cylinder.height / 2, -cylinder.radius};
    math::float3 max = {cylinder.radius, cylinder.height / 2, cylinder.radius};

    return {min, max};
}

world::ecs::AABB world::ecs::bounds(world::Plane plane) {
    math::float3 min = {-plane.width / 2, 0, -plane.depth / 2};
    math::float3 max = {plane.width / 2, 0, plane.depth / 2};

    return {min, max};
}

world::ecs::AABB world::ecs::bounds(world::Wedge wedge) {
    math::float3 min = {-wedge.width / 2, -wedge.height / 2, -wedge.depth / 2};
    math::float3 max = {wedge.width / 2, wedge.height / 2, wedge.depth / 2};

    return {min, max};
}

world::ecs::AABB world::ecs::bounds(world::Capsule capsule) {
    math::float3 min = {-capsule.radius, -capsule.height / 2, -capsule.radius};
    math::float3 max = {capsule.radius, capsule.height / 2, capsule.radius};

    return {min, max};
}
