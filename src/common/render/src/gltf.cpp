#include "render/render.hpp"

#include "fastgltf/core.hpp"

#include "logs/logs.hpp"

#include "fmt/std.h" // IWYU pragma: keep

using namespace sm;
using namespace sm::render;

namespace fg = fastgltf;

static auto gSink = logs::get_sink(logs::Category::eAssets);

static constexpr auto kExt = fg::Extensions::None;
static constexpr auto kOptions
    = fg::Options::DontRequireValidAssetMember
    | fg::Options::DecomposeNodeMatrices
    | fg::Options::GenerateMeshIndices;

bool Context::load_gltf_scene(Scene& scene, const fs::path& path) {
    if (!fs::exists(path)) {
        gSink.error("GLTF file does not exist: {}", path);
        return false;
    }

    fg::Parser parser{kExt};
    fg::GltfDataBuffer data;

    data.loadFromFile(path);

    auto asset = parser.loadGltf(&data, path.parent_path(), kOptions, fg::Category::OnlyRenderable);

    return true;
}
