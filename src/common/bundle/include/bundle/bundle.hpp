#pragma once

#include "core/vector.hpp"

#include "logs/logs.hpp"

#include "core/map.hpp"
#include "core/text.hpp"

#include "service/freetype.hpp"

#include "bundle.reflect.h"

#include <span>

namespace sm::bundle {
    class AssetBundle;
    class Font;

    using AssetSink = logs::Sink<logs::Category::eAssets>;
    using BinaryData = sm::Vector<uint8_t>;
    using BinarySpan = std::span<const uint8_t>;

    class Font {
        friend class AssetBundle;

        const AssetSink& m_log;
        const char *m_name;
        BinaryData m_data;
        FT_Face m_face;

        Font(const AssetSink& log, BinaryData data, const char *name);

        Font(const Font&) = delete;
    public:
        Font(Font&& other)
            : m_log(other.m_log)
            , m_name(other.m_name)
            , m_data(std::move(other.m_data))
            , m_face(other.m_face)
        {
            other.m_face = nullptr;
        }

        ~Font();
    };

    class AssetBundle {
        const char *m_root;

        AssetSink m_log;

        sm::Map<sm::String, BinaryData> m_shaders;
        sm::Map<sm::String, Font> m_fonts;

    public:
        AssetBundle(const char *root, logs::ILogger& logger)
            : m_root(root)
            , m_log(logger)
        { }

        AssetBundle(const AssetBundle&) = delete;
        AssetBundle(AssetBundle&&) = delete;

        BinarySpan load_shader(const char *path);
        Font& load_font(const char *path);
    };
}
