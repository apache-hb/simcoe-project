#include "render/draw.hpp"

#include "core/map.hpp"

#include "math/hash.hpp" // IWYU pragma: keep

using namespace sm;
using namespace sm::draw;

template <>
struct std::hash<Vertex> {
    constexpr auto operator()(const Vertex &vertex) const {
        size_t hash = 0;
        sm::hash_combine(hash, vertex.position);
        sm::hash_combine(hash, vertex.colour);
        return hash;
    }
};

template<>
struct std::equal_to<Vertex> {
    constexpr bool operator()(const Vertex &lhs, const Vertex &rhs) const {
        return lhs.position == rhs.position && lhs.colour == rhs.colour;
    }
};

struct PrimitiveBuilder {
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
		//.normal = ((v0.normal + v1.normal) / 2.f).normalized(),
		//.uv = ((v0.uv + v1.uv) / 2.f).normalized(),
		.colour = (v0.colour + v1.colour) / 2,
	};
}

static void subdivide(Mesh& mesh) {
    auto& [bounds, vertices, indices] = mesh;
    sm::Vector<uint16> old_indices = indices;
    sm::Vector<Vertex> old_vertices = vertices;

    indices.clear();
    vertices.clear();

    uint32 count = uint32(old_indices.size() / 3);
    for (uint32 i = 0; i < count; i++)
    {
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

        indices.push_back(i* 6 + 0);
        indices.push_back(i* 6 + 3);
        indices.push_back(i* 6 + 5);

        indices.push_back(i* 6 + 3);
        indices.push_back(i* 6 + 4);
        indices.push_back(i* 6 + 5);

        indices.push_back(i* 6 + 5);
        indices.push_back(i* 6 + 4);
        indices.push_back(i* 6 + 2);

        indices.push_back(i* 6 + 3);
        indices.push_back(i* 6 + 1);
        indices.push_back(i* 6 + 4);
    }
}

static Mesh cube(const Cube &cube) {
    auto [w, h, d] = cube;

    PrimitiveBuilder builder;

    builder.quad({{-w, h, -d}}, {{-w, h, d}}, {{-w, -h, d}}, {{-w, -h, -d}}); // back

    builder.quad({{w, -h, -d}}, {{w, -h, d}}, {{w, h, d}}, {{w, h, -d}}); // front

    builder.quad({{-w, -h, -d}}, {{-w, -h, d}}, {{w, -h, d}}, {{w, -h, -d}}); // left

    builder.quad({{w, h, -d}}, {{w, h, d}}, {{-w, h, d}}, {{-w, h, -d}}); // right

    builder.quad({{w, -h, -d}}, {{w, h, -d}}, {{-w, h, -d}}, {{-w, -h, -d}}); // bottom

    builder.quad({{-w, -h, d}}, {{-w, h, d}}, {{w, h, d}}, {{w, -h, d}}); // top

    return builder.build();
}

static Mesh cylinder(const Cylinder& cylinder) {
    auto [radius, height, slices] = cylinder;
    PrimitiveBuilder builder;

    for (int i = 0; i < slices; ++i) {
        float theta0 = float(i) * 2 * 3.14f / float(slices);
        float theta1 = float(i + 1) * 2 * 3.14f / float(slices);

        // make top
        {
            Vertex v0 = { float3(radius * cos(theta0), height / 2, radius * sin(theta0)) };
            Vertex v1 = { float3(radius * cos(theta1), height / 2, radius * sin(theta1)) };
            Vertex v2 = { float3(0, height / 2, 0) };

            builder.triangle(v0, v1, v2);
        }

        // make bottom
        {
            Vertex v0 = { float3(radius * cos(theta0), -height / 2, radius * sin(theta0)) };
            Vertex v1 = { float3(radius * cos(theta1), -height / 2, radius * sin(theta1)) };
            Vertex v2 = { float3(0, -height / 2, 0) };

            builder.triangle(v0, v2, v1);
        }

        // make side
        {
            Vertex v0 = { float3(radius * cos(theta0), -height / 2, radius * sin(theta0)) };
            Vertex v1 = { float3(radius * cos(theta1), -height / 2, radius * sin(theta1)) };
            Vertex v2 = { float3(radius * cos(theta0), height / 2, radius * sin(theta0)) };
            Vertex v3 = { float3(radius * cos(theta1), height / 2, radius * sin(theta1)) };

            builder.quad(v0, v1, v3, v2);
        }
    }

    return builder.build();
}

static Mesh wedge(const Wedge& wedge) {
    auto [width, height, depth] = wedge;
    PrimitiveBuilder builder;

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

static Mesh geosphere(const GeoSphere& geosphere) {
    auto [radius, tessellation] = geosphere;
    PrimitiveBuilder builder;

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

	uint16 k[60] =
	{
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

	for (auto& v : mesh.vertices)
	{
		//v.normal = v.position.normalized();
		v.position = v.position.normalized() * radius;
	}

	// redo the vertex winding
	for (size_t i = 0; i < mesh.indices.size(); i += 3)
		std::swap(mesh.indices[i], mesh.indices[i + 2]);

    return mesh;
}

Mesh draw::primitive(const MeshInfo &info) {
    switch (info.type.as_enum()) {
    case MeshType::eCube: return cube(info.cube);
    case MeshType::eCylinder: return cylinder(info.cylinder);
    case MeshType::eWedge: return wedge(info.wedge);
    case MeshType::eGeoSphere: return geosphere(info.geosphere);

    default:
        using Reflect = ctu::TypeInfo<MeshType>;
        NEVER("Unhandled primitive type %s", Reflect::to_string(info.type).data());
    }
}
