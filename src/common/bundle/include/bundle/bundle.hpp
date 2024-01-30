#pragma once

#include "core/vector.hpp"

#include "logs/logs.hpp"

#include "bundle.reflect.h"

namespace sm::bundle {
    class AssetBundle {
        logs::Sink<logs::Category::eAssets> m_log;
        const char *m_root;

    public:
        AssetBundle(const char *root, logs::ILogger& logger)
            : m_log(logger)
            , m_root(root)
        { }

        AssetBundle(const AssetBundle&) = delete;
        AssetBundle(AssetBundle&&) = delete;

        sm::Vector<uint8_t> load_shader(const char *path) const;
    };
}
