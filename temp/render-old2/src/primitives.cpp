#include "render/mesh.hpp"

#include "core/map.hpp"

#include "math/hash.hpp" // IWYU pragma: keep

using namespace sm;
using namespace sm::draw;

template <>
struct std::hash<Vertex> {
    constexpr auto operator()(const Vertex &vertex) const {
        size_t hash = 0;
        sm::hash_combine(hash, vertex.position);
        sm::hash_combine(hash, vertex.uv);
        return hash;
    }
};

template<>
struct std::equal_to<Vertex> {
    constexpr bool operator()(const Vertex &lhs, const Vertex &rhs) const {
        return lhs.position == rhs.position && lhs.uv == rhs.uv;
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

Mesh draw::primitive(const MeshInfo &info) {
    switch (info.type) {
    case MeshType::eCube:
        return cube(info.cube);

    default:
        using Reflect = ctu::TypeInfo<MeshType>;
        CT_NEVER("Unhandled primitive type %s", Reflect::to_string(info.type).data());
    }
}
