#include "stdafx.hpp"

#include "world/world.hpp"

using namespace sm;
using namespace sm::world;

static auto gSink = logs::get_sink(logs::Category::eAssets);

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

WorldInfo world::empty_world(sm::StringView name) {
    WorldInfo world = {
        .name = sm::String{name}
    };

    world.nodes.push_back({
        .name = "Root",
        .transform = default_transform()
    });

    return world;
}

#define BADPARSE(...) do { gSink.error(__VA_ARGS__); return false; } while (false)

struct WorldLoad {
    Archive& archive;
    WorldInfo mWorld;

    template<typename T, typename F>
    bool load_vector(sm::Vector<T>& vec, sm::StringView name, F&& func) {
        size_t count;
        if (!archive.read_length(count))
            BADPARSE("Failed to read count for {}", name);

        vec.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            T item;
            if (!std::invoke(func, *this, item, (uint16)i))
                BADPARSE("Failed to read item {} for {}", i, name);

            vec.push_back(std::move(item));
        }

        if (vec.size() != count)
            BADPARSE("Failed to read all items for {}", name);

        return true;
    }

    bool load_node(NodeInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            BADPARSE("Failed to read node name");

        if (!archive.read_many(it.children))
            BADPARSE("Failed to read children for node {}", it.name);

        if (!archive.read_many(it.objects))
            BADPARSE("Failed to read objects for node {}", it.name);

        return true;
    }

    bool load_camera(CameraInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            BADPARSE("Failed to read camera name");

        if (!archive.read(it.position))
            BADPARSE("Failed to read camera position for {}", it.name);

        if (!archive.read(it.direction))
            BADPARSE("Failed to read camera direction for {}", it.name);

        return true;
    }

    bool load_object(ObjectInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            BADPARSE("Failed to read object name");

        if (!archive.read(it.info))
            BADPARSE("Failed to read object info for {}", it.name);

        return true;
    }

    bool load_material(MaterialInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            BADPARSE("Failed to read material name");

        return true;
    }

    bool load_image(ImageInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            BADPARSE("Failed to read image name");

        return true;
    }

    bool load_buffer(BufferInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            BADPARSE("Failed to read buffer name");

        if (!archive.read_many(it.data))
            BADPARSE("Failed to read buffer data for {}", it.name);

        return true;
    }

    bool load_version0() {
        if (!archive.read_string(mWorld.name))
            BADPARSE("Failed to read world name");

        if (!archive.read(mWorld.root_node))
            BADPARSE("Failed to read root node");

        if (!archive.read(mWorld.active_camera))
            BADPARSE("Failed to read active camera");

        if (!archive.read(mWorld.default_material))
            BADPARSE("Failed to read default material");

        if (!load_vector(mWorld.nodes, "nodes", &WorldLoad::load_node))
            return false;

        if (!load_vector(mWorld.cameras, "cameras", &WorldLoad::load_camera))
            return false;

        if (!load_vector(mWorld.objects, "objects", &WorldLoad::load_object))
            return false;

        if (!load_vector(mWorld.materials, "materials", &WorldLoad::load_material))
            return false;

        if (!load_vector(mWorld.images, "images", &WorldLoad::load_image))
            return false;

        if (!load_vector(mWorld.buffers, "buffers", &WorldLoad::load_buffer))
            return false;

        return true;
    }
};

bool world::load_world(WorldInfo& info, Archive &archive) {
    WorldLoad load{archive};
    if (!load.load_version0())
        return false;

    info = std::move(load.mWorld);

    return true;
}

template<typename T, typename F>
static void write_many(Archive& archive, const sm::Vector<T>& data, F&& func) {
    archive.write_length(data.size());
    for (size_t i = 0; i < data.size(); ++i)
        std::invoke(func, data[i]);
}

void world::save_world(Archive& archive, const WorldInfo& world) {
    archive.write_string(world.name);
    archive.write(world.root_node);
    archive.write(world.active_camera);
    archive.write(world.default_material);

    write_many(archive, world.nodes, [&](const NodeInfo& it) {
        archive.write_string(it.name);
        archive.write_many(it.children);
        archive.write_many(it.objects);
    });

    write_many(archive, world.cameras, [&](const CameraInfo& it) {
        archive.write_string(it.name);
        archive.write(it.position);
        archive.write(it.direction);
    });

    write_many(archive, world.objects, [&](const ObjectInfo& it) {
        archive.write_string(it.name);
        archive.write(it.info);
    });

    write_many(archive, world.materials, [&](const MaterialInfo& it) {
        archive.write_string(it.name);
    });

    write_many(archive, world.images, [&](const ImageInfo& it) {
        archive.write_string(it.name);
    });

    write_many(archive, world.buffers, [&](const BufferInfo& it) {
        archive.write_string(it.name);
        archive.write_many(it.data);
    });
}
