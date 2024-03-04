#include "world/world.hpp"

#include "logs/logs.hpp"

using namespace sm;
using namespace sm::world;

#define badparse(...) do { mSink.error(__VA_ARGS__); return false; } while (false)

struct WorldLoad {
    Archive& archive;
    logs::Sink mSink;
    WorldInfo mWorld;

    template<typename T, typename F>
    bool load_vector(sm::Vector<T>& vec, sm::StringView name, F&& func) {
        size_t count;
        if (!archive.read_length(count))
            badparse("Failed to read count for {}", name);

        vec.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            T item;
            if (!std::invoke(func, *this, item, (uint16)i))
                badparse("Failed to read item {} for {}", i, name);

            vec.push_back(std::move(item));
        }

        if (vec.size() != count)
            badparse("Failed to read all items for {}", name);

        return true;
    }

    bool load_node(NodeInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            badparse("Failed to read node name");

        if (!archive.read_many(it.children))
            badparse("Failed to read children for node {}", it.name);

        if (!archive.read_many(it.objects))
            badparse("Failed to read objects for node {}", it.name);

        return true;
    }

    bool load_camera(CameraInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            badparse("Failed to read camera name");

        if (!archive.read(it.position))
            badparse("Failed to read camera position for {}", it.name);

        if (!archive.read(it.direction))
            badparse("Failed to read camera direction for {}", it.name);

        return true;
    }

    bool load_object(ObjectInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            badparse("Failed to read object name");

        if (!archive.read(it.info))
            badparse("Failed to read object info for {}", it.name);

        return true;
    }

    bool load_material(MaterialInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            badparse("Failed to read material name");

        return true;
    }

    bool load_image(ImageInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            badparse("Failed to read image name");

        return true;
    }

    bool load_buffer(BufferInfo& it, uint16 index) {
        if (!archive.read_string(it.name))
            badparse("Failed to read buffer name");

        if (!archive.read_many(it.data))
            badparse("Failed to read buffer data for {}", it.name);

        return true;
    }

    bool load_version0() {
        if (!archive.read_string(mWorld.name))
            badparse("Failed to read world name");

        if (!archive.read(mWorld.root_node))
            badparse("Failed to read root node");

        if (!archive.read(mWorld.active_camera))
            badparse("Failed to read active camera");

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

WorldInfo world::load_world(Archive &archive) {
    WorldLoad load{archive, logs::get_sink(logs::Category::eAssets)};
    if (!load.load_version0())
        return {};

    return load.mWorld;
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
