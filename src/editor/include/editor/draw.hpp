#pragma once

#include "draw/camera.hpp"

#include "render/render.hpp"

#include "editor.reflect.h"

namespace sm::ed {
    struct ItemIndex {
        ItemType type;
        uint16 index;

        constexpr auto operator<=>(const ItemIndex&) const = default;
        constexpr bool has_selection() const { return type != ed::ItemType::eNone; }

        static constexpr ItemIndex none() { return {ItemType::eNone, 0}; }
    };

    struct EditorContext final : public render::Context {
        using Super = render::Context;
        using Super::Super;

        EditorContext(const render::RenderConfig& config);

        void on_create() override;
        void on_destroy() override;

        void setup_framegraph(graph::FrameGraph& graph) override;

        void tick(float dt);

        void render();

        size_t add_camera();

        std::optional<world::AnyIndex> selected;
        render::SrvIndex index;
        input::InputService input;

        struct CameraData {
            sm::UniquePtr<draw::Camera> camera;
            graph::Handle target;
        };

        sm::Vector<CameraData> cameras;
        size_t active = 0;

        // should the device be recreated?
        bool recreate_device = false;

    private:
        void push_camera(size_t index, math::uint2 size);
        void imgui(graph::FrameGraph& graph, graph::Handle target);
    };
}
