#include "stdafx.hpp"

#include "world/world.hpp"

using namespace sm;
using namespace sm::world;

using namespace sm::math::literals;

BoxBounds world::computeObjectBounds(world::World& world, const Object& object) {
    BoxBounds bounds = { .min = FLT_MAX, .max = -FLT_MAX };

    auto buffer = world.get(world::get<Buffer>(object.vertices.source));

    for (const Vertex& vertex : buffer.view<Vertex>(object.vertices.offset, object.vertices.source_size / sizeof(Vertex))) {
        bounds.min = math::min(bounds.min, vertex.position);
        bounds.max = math::max(bounds.max, vertex.position);
    }

    return bounds;
}

Transform world::computeNodeTransform(world::World& world, IndexOf<Node> node) {
    const Node& info = world.get(node);
    if (info.parent == kInvalidIndex) {
        return info.transform;
    }

    return info.transform * computeNodeTransform(world, info.parent);
}

DXGI_FORMAT Object::getIndexBufferFormat() const {
    return (indexBufferFormat == IndexSize::eShort)
        ? DXGI_FORMAT_R16_UINT
        : DXGI_FORMAT_R32_UINT;
}

VertexFlags Model::getVertexBufferFlags() const {
    if (const Object *object = std::get_if<Object>(&mesh)) {
        return object->vertexBufferFlags;
    }

    return primitiveVertexBufferFlags();
}

DXGI_FORMAT Model::getIndexBufferFormat() const {
    if (const Object *object = std::get_if<Object>(&mesh)) {
        return object->getIndexBufferFormat();
    }

    return primitiveIndexBufferFormat();
}

#if 0
uint16 WorldInfo::add_node(const NodeInfo& info) {
    auto index = int_cast<uint16>(nodes.size());
    uint16 parent = info.parent;
    nodes.emplace_back(info);
    nodes[parent].children.push_back(index);
    return index;
}

uint16 WorldInfo::add_camera(const CameraInfo& info) {
    auto index = int_cast<uint16>(cameras.size());
    cameras.push_back(info);
    return index;
}

uint16 WorldInfo::add_object(const ObjectInfo& info) {
    auto index = int_cast<uint16>(objects.size());
    objects.push_back(info);
    return index;
}

uint16 WorldInfo::add_material(const MaterialInfo& info) {
    auto index = int_cast<uint16>(materials.size());
    materials.push_back(info);
    return index;
}

void WorldInfo::reparent_node(uint16 node, uint16 parent) {
    uint16 old_parent = nodes[node].parent;
    if (old_parent == parent) return;

    // remove this node from the old parent
    auto& children = nodes[old_parent].children;
    children.erase(std::remove(children.begin(), children.end(), node), children.end());

    // add this node to the new parent
    nodes[node].parent = parent;
    nodes[parent].children.push_back(node);
}

void WorldInfo::delete_node(uint16 index) {
    CTASSERTF(!is_root_node(index), "Cannot delete root node");
    auto& node = nodes[index];
    auto& parent = nodes[node.parent];
    auto& children = parent.children;

    // remove this node from the parent
    children.erase(std::remove(children.begin(), children.end(), index), children.end());

    // move all children to the parent
    for (uint16 child : node.children) {
        children.push_back(child);
    }
}

bool WorldInfo::is_root_node(uint16 node) const {
    return node == root_node;
}

void WorldInfo::add_node_object(uint16 node, uint16 object) {
    nodes[node].objects.push_back(object);
}
#endif

void World::moveNode(IndexOf<Node> node, IndexOf<Node> parent) {
    auto& node_ref = get(node);
    auto& parent_ref = get(parent);

    if (node_ref.parent == parent) return;

    if (node_ref.parent != kInvalidIndex) {
        auto& old_parent = get(node_ref.parent);
        auto& children = old_parent.children;
        children.erase(std::remove(children.begin(), children.end(), node), children.end());
    }

    node_ref.parent = parent;
    parent_ref.children.push_back(node);
}

void World::cloneNode(IndexOf<Node> node, IndexOf<Node> parent) {
    auto cloned = clone(node);
    auto& clone_ref = get(cloned);

    auto& parent_ref = get(parent);
    clone_ref.parent = parent;
    parent_ref.children.push_back(cloned);
}

IndexOf<Node> World::addNode(Node&& value) {
    IndexOf parent = value.parent;
    IndexOf it = add(std::move(value));
    get(parent).children.push_back(it);
    return it;
}

World world::default_world(sm::String name) {
    World world = {
        .name = std::move(name)
    };

    IndexOf mat = world.add(Material{
        .name = "Default",
        .albedo = math::float3(0.5f, 0.5f, 0.5f)
    });

    IndexOf cube = world.add(Model{
        .name = "Cube",
        .mesh = Cube{1, 1, 1},
        .material = mat
    });

    IndexOf root = world.add(Node{
        .name = "Scene Root",
        .transform = default_transform(),
        .models = { cube }
    });

    IndexOf camera = world.add(Camera{
        .name = "Camera",
        .position = { -3.f, 0.f, 0.f },
        .direction = world::kVectorForward,
        .fov = 75._deg
    });

    IndexOf scene = world.add(Scene{
        .name = "Scene",
        .root = root,
        .camera = camera
    });

    world.default_scene = scene;
    world.default_material = mat;

    return world;
}
