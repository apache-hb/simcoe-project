#include "archive/bundle.hpp"

#include "archive/io.hpp"

#include "core/arena.hpp"
#include "core/memory.hpp"
#include "core/text.hpp"
#include "core/format.hpp" // IWYU pragma: keep

#include "logs/logs.hpp"

#include "artery-font/artery-font.h"
#include "stb/stb_image.h"

#include "fs/fs.h"
#include "tar/tar.h"

using namespace sm;

static logs::Sink gSink = logs::get_sink(logs::Category::eAssets);

Bundle::Bundle(fs_t *vfs)
    : mFileSystem(vfs)
{ }

Bundle::Bundle(io_t *stream, archive::BundleFormat type)
    : Bundle(fs_virtual("bundle", sm::global_arena()))
{
    if (type != archive::BundleFormat::eTar) {
        gSink.warn("unsupported bundle format: {}", type);
        return;
    }

    if (tar_result_t err = tar_extract(*mFileSystem, stream); err.error != eTarOk) {
        gSink.error("failed to extract bundle: {}", tar_error_string(err.error));
    }
}

sm::Span<const uint8> Bundle::get_file(sm::StringView dir, sm::StringView name) const {
    sm::String path = fmt::format("bundle/{}/{}", dir, name);
    IoHandle file = fs_open(*mFileSystem, path.c_str(), eOsAccessRead);

    if (OsError err = io_error(*file); err.failed()) {
        gSink.error("failed to open file: {}", err);
        return {};
    }

    size_t size = io_size(*file);
    const uint8 *data = (uint8*)io_map(*file, eOsProtectRead);

    sm::Memory mem = size;
    gSink.info("loaded file: {} ({} bytes)", path, mem);

    return { data, size };
}

ShaderIr Bundle::get_shader_bytecode(const char *name) const {
    return get_file("shaders", fmt::format("{}.cso", name));
}

TextureData Bundle::get_texture(const char *name) const {
    return get_file("textures", fmt::format("{}.dds", name));
}

static int wrap_io_read(void *dst, int length, void *user) {
    io_t *io = (io_t*)user;
    return int_cast<int>(io_read(io, dst, int_cast<size_t>(length)));
}

// i have some choice words for artery-fonts api design
template<typename T>
struct ArteryVector : sm::Vector<T> {
    using Super = sm::Vector<T>;
    using Super::Super;

    operator T*() { return Super::data(); }
};

using ArteryByteArray = ArteryVector<unsigned char>;

FontInfo Bundle::get_font(const char *name) const {
    auto texture = get_file("fonts", fmt::format("{}.png", name));

    int width, height, channels;
    stbi_uc *pixels = stbi_load_from_memory(texture.data(), texture.size(), &width, &height, &channels, 4);

    if (pixels == nullptr) {
        gSink.error("failed to load font texture: {}", stbi_failure_reason());
        return {};
    }

    gSink.info("loaded font texture: {} ({}x{})", name, width, height);

    sm::String path = fmt::format("bundle/fonts/{}.arfont", name);
    IoHandle file = fs_open(*mFileSystem, path.c_str(), eOsAccessRead);

    if (OsError err = io_error(*file); err.failed()) {
        gSink.error("failed to open file: {}", err);
        return {};
    }

    using Artery = artery_font::ArteryFont<float, ArteryVector, ArteryByteArray, sm::String>;

    Artery font;
    auto result = artery_font::decode<wrap_io_read>(font, (void*)file.get());
    if (!result) {
        gSink.error("failed to decode font: {}", name);
        return {};
    }

    gSink.info("loaded font: {}", name);

    return {};
}
