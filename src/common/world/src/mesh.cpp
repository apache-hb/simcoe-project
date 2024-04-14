#include "stdafx.hpp"

#include "world/mesh.hpp"

using namespace sm;
using namespace sm::world;

static constexpr VertexFlags kPrimitiveVertexFlags = VertexFlags::none();

Transform world::default_transform() {
    Transform result = {
        .position = 0.f,
        .rotation = quatf::identity(),
        .scale = 1.f
    };

    return result;
}

float4x4 Transform::matrix() const {
    return float4x4::transform(position, rotation, scale);
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

float3 BoxBounds::getCenter() const {
    return (min + max) / 2.f;
}

float3 BoxBounds::getExtents() const {
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
    BoxBounds bounds = {.min = FLT_MAX, .max = FLT_MIN};

    // TODO: probably want to merge verticies that are nearly the same
    sm::HashMap<Vertex, uint16> lookup;
    sm::Vector<Vertex> vertices;
    sm::Vector<uint16> indices;

    uint16 addVertex(const Vertex &vertex) {
        auto [it, inserted] = lookup.try_emplace(vertex, uint16(vertices.size()));
        if (inserted) {
            bounds.min = math::min(bounds.min, vertex.position);
            bounds.max = math::max(bounds.max, vertex.position);

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
            .bounds = bounds,
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
    auto& [bounds, vertices, indices] = mesh;
    sm::Vector<uint16> oldIndices = indices;
    sm::Vector<Vertex> oldVertices = vertices;

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
        {.position = {-w, h, -d}, .uv = {0, 1}},
        {.position = {-w, h, d}, .uv = {0, 0}},
        {.position = {-w, -h, d}, .uv = {1, 0}},
        {.position = {-w, -h, -d}, .uv = {1, 1}}
    ); // back

    builder.quad(
        {.position = {w, -h, -d}, .uv = {0, 1}},
        {.position = {w, -h, d}, .uv = {0, 0}},
        {.position = {w, h, d}, .uv = {1, 0}},
        {.position = {w, h, -d}, .uv = {1, 1}}
    ); // front

    builder.quad(
        {.position = {-w, -h, -d}, .uv = {0, 1}},
        {.position = {-w, -h, d}, .uv = {0, 0}},
        {.position = {w, -h, d}, .uv = {1, 0}},
        {.position = {w, -h, -d}, .uv = {1, 1}}
    ); // left

    builder.quad(
        {.position = {w, h, -d}, .uv = {0, 1}},
        {.position = {w, h, d}, .uv = {0, 0}},
        {.position = {-w, h, d}, .uv = {1, 0}},
        {.position = {-w, h, -d}, .uv = {1, 1}}
    ); // right

    builder.quad(
        {.position = {w, -h, -d}, .uv = {0, 1}},
        {.position = {w, h, -d}, .uv = {0, 0}},
        {.position = {-w, h, -d}, .uv = {1, 0}},
        {.position = {-w, -h, -d}, .uv = {1, 1}}
    ); // bottom

    builder.quad(
        {.position = {-w, -h, d}, .uv = {0, 1}},
        {.position = {-w, h, d}, .uv = {0, 0}},
        {.position = {w, h, d}, .uv = {1, 0}},
        {.position = {w, -h, d}, .uv = {1, 1}}
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

            float3 normal = {dx, dy, dz};
            float2 uv = {u, v};
            float3 position = normal * radius;

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
                .position = float3(radius * c0, height / 2, radius * s0),
                .uv = float2(float(i) / float(slices), 0)
            };
            Vertex v1 = {
                .position = float3(radius * c1, height / 2, radius * s1),
                .uv = float2(float(i + 1) / float(slices), 0)
            };
            Vertex v2 = {
                .position = float3(0, height / 2, 0),
                .uv = float2(0.5f, 0)
            };

            builder.triangle(v0, v1, v2);
        }

        // make bottom half of the cylinder
        {
            Vertex v0 = {
                .position = float3(radius * c0, -height / 2, radius * s0),
                .uv = float2(float(i) / float(slices), 1)
            };
            Vertex v1 = {
                .position = float3(radius * c1, -height / 2, radius * s1),
                .uv = float2(float(i + 1) / float(slices), 1)
            };
            Vertex v2 = {
                .position = float3(0, -height / 2, 0),
                .uv = float2(0.5f, 1)
            };

            builder.triangle(v0, v2, v1);
        }

        // make body
        {
            Vertex v0 = {
                .position = float3(radius * c0, -height / 2, radius * s0),
                .uv = float2(float(i) / float(slices), 0)
            };
            Vertex v1 = {
                .position = float3(radius * c1, -height / 2, radius * s1),
                .uv = float2(float(i + 1) / float(slices), 0)
            };
            Vertex v2 = {
                .position = float3(radius * c0, height / 2, radius * s0),
                .uv = float2(float(i) / float(slices), 1)
            };
            Vertex v3 = {
                .position = float3(radius * c1, height / 2, radius * s1),
                .uv = float2(float(i + 1) / float(slices), 1)
            };

            builder.quad(v0, v1, v3, v2);
        }
    }

    return builder.build();
}

static Mesh plane(const world::Plane& plane) {
    auto [width, depth] = plane;
    MeshBuilder builder;

    // positions
    float3 p0 = { -width / 2, 0, -depth / 2 };
    float3 p1 = { width / 2, 0, -depth / 2 };
    float3 p2 = { width / 2, 0, depth / 2 };
    float3 p3 = { -width / 2, 0, depth / 2 };

    // normal
    float3 n = { 0, 1, 0 };

    // uv coordinates
    float2 uv0 = { 0, 0 };
    float2 uv1 = { 1, 0 };
    float2 uv2 = { 1, 1 };
    float2 uv3 = { 0, 1 };

    // tangent
    float3 t = { 1, 0, 0 };

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
        Vertex v0 = { float3(-w2, -h2, d2), };
        Vertex v1 = { float3(w2, -h2, d2), };
        Vertex v2 = { float3(w2, -h2, -d2), };
        Vertex v3 = { float3(-w2, -h2, -d2), };

        builder.quad(v0, v1, v2, v3);
    }

    // bottom quad
    {
        Vertex v0 = { float3(-w2, -h2, d2), };
        Vertex v1 = { float3(w2, -h2, d2), };
        Vertex v2 = { float3(w2, h2, -d2), };
        Vertex v3 = { float3(-w2, h2, -d2), };

        builder.quad(v3, v2, v1, v0);
    }

    // left tri
    {
        Vertex v0 = { float3(-w2, -h2, d2), };
        Vertex v1 = { float3(-w2, -h2, -d2), };
        Vertex v2 = { float3(-w2, h2, -d2), };

        builder.triangle(v0, v1, v2);
    }

    // right tri
    {
        Vertex v0 = { float3(w2, -h2, d2), };
        Vertex v1 = { float3(w2, h2, -d2), };
        Vertex v2 = { float3(w2, -h2, -d2), };

        builder.triangle(v0, v1, v2);
    }

    // front quad
    {
        Vertex v0 = { float3(-w2, -h2, -d2), };
        Vertex v1 = { float3(w2, -h2, -d2), };
        Vertex v2 = { float3(w2, h2, -d2), };
        Vertex v3 = { float3(-w2, h2, -d2), };

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
                Vertex v0 = { float3(radius * cp0 * c0, height / 2 + radius * sp0, radius * cp0 * s0) };
                Vertex v1 = { float3(radius * cp1 * c0, height / 2 + radius * sp1, radius * cp1 * s0) };
                Vertex v2 = { float3(radius * cp0 * c1, height / 2 + radius * sp0, radius * cp0 * s1) };
                Vertex v3 = { float3(radius * cp1 * c1, height / 2 + radius * sp1, radius * cp1 * s1) };

                builder.quad(v0, v1, v3, v2);
            }

            // make bottom cap of the capsule
            {
                Vertex v0 = { float3(radius * cp0 * c0, -height / 2 - radius * sp0, radius * cp0 * s0) };
                Vertex v1 = { float3(radius * cp1 * c0, -height / 2 - radius * sp1, radius * cp1 * s0) };
                Vertex v2 = { float3(radius * cp0 * c1, -height / 2 - radius * sp0, radius * cp0 * s1) };
                Vertex v3 = { float3(radius * cp1 * c1, -height / 2 - radius * sp1, radius * cp1 * s1) };

                builder.quad(v0, v2, v3, v1);
            }
        }

        // make body
        {
            Vertex v0 = { float3(radius * c0, -height / 2, radius * s0) };
            Vertex v1 = { float3(radius * c1, -height / 2, radius * s1) };
            Vertex v2 = { float3(radius * c0, height / 2, radius * s0) };
            Vertex v3 = { float3(radius * c1, height / 2, radius * s1) };

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
		Vertex v2 = { float3(0, 0, -d2), };
		Vertex v1 = { float3(-w2, 0, 0), };
		Vertex v0 = { float3(0, h2, 0), };

		builder.triangle(v0, v1, v2);
	}

	{
		Vertex v0 = { float3(0, 0, -d2), };
		Vertex v2 = { float3(0, h2, 0), };
		Vertex v1 = { float3(w2, 0, 0), };

		builder.triangle(v0, v1, v2);
	}

	{
		Vertex v0 = { float3(0, 0, -d2), };
		Vertex v2 = { float3(-w2, 0, 0), };
		Vertex v1 = { float3(0, -h2, 0), };

		builder.triangle(v0, v2, v1);
	}

	{
		Vertex v0 = { float3(0, 0, -d2), };
		Vertex v2 = { float3(0, -h2, 0), };
		Vertex v1 = { float3(w2, 0, 0), };

		builder.triangle(v0, v2, v1);
	}

	// left side

	{
		Vertex v2 = { float3(0, 0, d2), };
		Vertex v0 = { float3(-w2, 0, 0), };
		Vertex v1 = { float3(0, h2, 0), };

		builder.triangle(v0, v1, v2);
	}

	{
		Vertex v0 = { float3(0, 0, d2), };
		Vertex v1 = { float3(0, h2, 0), };
		Vertex v2 = { float3(w2, 0, 0), };

		builder.triangle(v0, v1, v2);
	}

	{
		Vertex v0 = { float3(0, 0, d2), };
		Vertex v1 = { float3(-w2, 0, 0), };
		Vertex v2 = { float3(0, -h2, 0), };

		builder.triangle(v0, v2, v1);
	}

	{
		Vertex v0 = { float3(0, 0, d2), };
		Vertex v1 = { float3(0, -h2, 0), };
		Vertex v2 = { float3(w2, 0, 0), };

		builder.triangle(v0, v2, v1);
	}

    return builder.build();
}

static Mesh geosphere(const world::GeoSphere& geosphere) {
    auto [radius, tessellation] = geosphere;
    MeshBuilder builder;

	sm::Vector<Vertex> vertices;
	sm::Vector<uint16> indices;

	// Create a icosahedron
	const float x = 0.525731f;
	const float z = 0.850651f;

	vertices.push_back({ float3(-x, 0, z)  * radius });
	vertices.push_back({ float3(x, 0, z)   * radius });
	vertices.push_back({ float3(-x, 0, -z) * radius });
	vertices.push_back({ float3(x, 0, -z)  * radius });

	vertices.push_back({ float3(0, z, x)   * radius });
	vertices.push_back({ float3(0, z, -x)  * radius });
	vertices.push_back({ float3(0, -z, x)  * radius });
	vertices.push_back({ float3(0, -z, -x) * radius });

	vertices.push_back({ float3(z, x, 0)   * radius });
	vertices.push_back({ float3(-z, x, 0)  * radius });
	vertices.push_back({ float3(z, -x, 0)  * radius });
	vertices.push_back({ float3(-z, -x, 0) * radius });

	static constexpr uint16 kIndices[60] = {
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	indices.assign(std::begin(kIndices), std::end(kIndices));

	builder.indices = indices;
	builder.vertices = vertices;

	auto mesh = builder.build();

	for (int i = 0; i < tessellation; i++)
		subdivide(mesh);

	for (auto& v : mesh.vertices) {
        float3 normal = v.position.normalized();
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
