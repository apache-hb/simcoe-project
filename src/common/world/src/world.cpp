#include "stdafx.hpp"

#include "world/world.hpp"

using namespace sm;
using namespace sm::world;

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

World world::empty_world(sm::String name) {
    World world = {
        .name = std::move(name)
    };

    IndexOf mat = world.add(Material{ .name = "Default", .albedo = float4(0.5f, 0.5f, 0.5f, 1.f) });

    IndexOf cube = world.add(Model{ .name = "Cube", .mesh = Cube{1, 1, 1}, .material = mat });

    IndexOf root = world.add(Node{ .name = "Scene Root", .transform = default_transform(), .models = { cube } });

    IndexOf camera = world.add(Camera{ .name = "Camera", .position = 0.f, .direction = float3(0.f, 1.f, 0.f) });

    IndexOf scene = world.add(Scene{ .name = "Scene", .root = root, .camera = camera });

    world.active_scene = scene;
    world.default_material = mat;

    return world;
}
