#pragma once

#include "ui/ui.hpp"
#include "render/render.hpp"

namespace sm::ui {
    struct CBUFFER_ALIGN CanvasProjection {
        math::float4x4 projection;
    };

    class CanvasCommands : public render::IRenderNode {
        // external data
        bundle::AssetBundle& m_assets;

        // canvas data
        ui::Canvas *m_canvas;

        // canvas pipeline
        static constexpr inline const char *kVertexShader = "shaders/canvas.vs.cso";
        static constexpr inline const char *kPixelShader = "shaders/canvas.ps.cso";
        static constexpr inline const char *kFontAtlasSlot = "font_atlas";
        static constexpr inline const char *kCameraSlot = "camera";

        rhi::PipelineState m_pipeline;

        D3D12_VIEWPORT m_viewport;
        D3D12_RECT m_scissor;

        // gpu resources

        rhi::ResourceObject m_vbo;
        rhi::ResourceObject m_ibo;

        /// !!! DO NOT READ FROM THESE POINTERS
        /// !!! MAY BE GPU UPLOAD HEAP, SLOW
        ui::Vertex *m_vbo_data = nullptr;
        ui::Index *m_ibo_data = nullptr;

        D3D12_VERTEX_BUFFER_VIEW m_vbo_view{};
        D3D12_INDEX_BUFFER_VIEW m_ibo_view{};

        uint32_t m_vbo_length = 0;
        uint32_t m_ibo_length = 0;

        // font atlas resources

        // TODO: extract this out
        bundle::Texture m_font_atlas_texture;
        rhi::ResourceObject m_atlas;
        render::SrvHeapIndex m_font_atlas_index = render::SrvHeapIndex::eInvalid;

        // camera resources, TODO: get root constants working to avoid this
        rhi::ResourceObject m_camera;
        render::SrvHeapIndex m_camera_index = render::SrvHeapIndex::eInvalid;
        CanvasProjection m_projection;
        CanvasProjection *m_projection_data = nullptr;

        void setup_pipeline(render::Context& context);

        void setup_buffers(render::Context& context, uint32_t vbo_length, uint32_t ibo_length);

        void setup_font_atlas(render::Context& context);
        void setup_camera(render::Context& context);
        void update_camera(const ui::BoxBounds& bounds);
        void update_viewport(const ui::BoxBounds& bounds);

        void resize(render::Context& context, math::uint2 size) override {
            m_canvas->set_screen(size);
            auto bounds = m_canvas->get_screen();

            update_camera(bounds);
            update_viewport(bounds);
        }

        void create(render::Context& context) override;
        void destroy(render::Context& context) override;
        void build(render::Context& context) override;

    public:
        CanvasCommands(ui::Canvas *canvas, bundle::AssetBundle& assets)
            : m_assets(assets)
            , m_canvas(canvas)
        { }

        void change_canvas(ui::Canvas *canvas) {
            m_canvas = canvas;
        }
    };
}
