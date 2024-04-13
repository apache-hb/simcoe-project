#include "stdafx.hpp"

#include "world/mesh.hpp"

using namespace sm;
using namespace sm::world;

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
    sm::Vector<uint16> old_indices = indices;
    sm::Vector<Vertex> old_vertices = vertices;

    indices.clear();
    vertices.clear();

    uint32 count = uint32(old_indices.size() / 3);
    for (uint32 i = 0; i < count; i++) {
        Vertex v0 = old_vertices[old_indices[i * 3 + 0]];
        Vertex v1 = old_vertices[old_indices[i * 3 + 1]];
        Vertex v2 = old_vertices[old_indices[i * 3 + 2]];

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

static Mesh sphere(const world::Sphere& sphere) {
    auto [radius, slices, stacks] = sphere;
    MeshBuilder builder;

    for (int i = 0; i < slices; i++) {
        float theta0 = float(i) * 2 * 3.14f / float(slices);
        float theta1 = float(i + 1) * 2 * 3.14f / float(slices);
        auto [s0, c0] = math::sincos(theta0);
        auto [s1, c1] = math::sincos(theta1);

        for (int j = 0; j < stacks; ++j) {
            float phi0 = float(j) * 3.14f / float(stacks);
            float phi1 = float(j + 1) * 3.14f / float(stacks);
            auto [sp0, cp0] = math::sincos(phi0);
            auto [sp1, cp1] = math::sincos(phi1);

            float3 p0 = { radius * cp0 * c0, radius * sp0, radius * cp0 * s0 };
            float3 p1 = { radius * cp1 * c0, radius * sp1, radius * cp1 * s0 };
            float3 p2 = { radius * cp0 * c1, radius * sp0, radius * cp0 * s1 };
            float3 p3 = { radius * cp1 * c1, radius * sp1, radius * cp1 * s1 };

            float2 uv0 = { float(i) / float(slices), float(j) / float(stacks) };
            float2 uv1 = { float(i + 1) / float(slices), float(j) / float(stacks) };
            float2 uv2 = { float(i) / float(slices), float(j + 1) / float(stacks) };
            float2 uv3 = { float(i + 1) / float(slices), float(j + 1) / float(stacks) };

            Vertex v0 = { p0, p0.normalized(), uv0 };
            Vertex v1 = { p1, p1.normalized(), uv1 };
            Vertex v2 = { p2, p2.normalized(), uv2 };
            Vertex v3 = { p3, p3.normalized(), uv3 };

            builder.quad(v0, v1, v3, v2);
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

	vertices.push_back({ .position = float3(-x, 0, z) * radius });
	vertices.push_back({ .position = float3(x, 0, z) * radius });
	vertices.push_back({ .position = float3(-x, 0, -z) * radius });
	vertices.push_back({ .position = float3(x, 0, -z) * radius });

	vertices.push_back({ .position = float3(0, z, x) * radius });
	vertices.push_back({ .position = float3(0, z, -x) * radius });
	vertices.push_back({ .position = float3(0, -z, x) * radius });
	vertices.push_back({ .position = float3(0, -z, -x) * radius });

	vertices.push_back({ .position = float3(z, x, 0) * radius });
	vertices.push_back({ .position = float3(-z, x, 0) * radius });
	vertices.push_back({ .position = float3(z, -x, 0) * radius });
	vertices.push_back({ .position = float3(-z, -x, 0) * radius });

	static constexpr uint16 k[60] = {
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	indices.assign(std::begin(k), std::end(k));

	builder.indices = indices;
	builder.vertices = vertices;

	auto mesh = builder.build();

	for (int i = 0; i < tessellation; i++)
		subdivide(mesh);

	for (auto& v : mesh.vertices) {
		//v.normal = v.position.normalized();
		v.position = v.position.normalized() * radius;
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
    CT_NEVER("Not implemented");
    //return ::plane(plane);
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
