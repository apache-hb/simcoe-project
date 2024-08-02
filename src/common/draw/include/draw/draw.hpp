#pragma once

#include "draw/forward_plus.hpp"

#include "render/core/graph.hpp"
#include "render/core/render.hpp"
#include "world/ecs.hpp"

namespace sm::draw {
    class Camera;

    namespace ecs {
        using ObjectDeviceData = render::ConstBuffer<ObjectData>;
        using ViewportDeviceData = render::ConstBuffer<ViewportData>;

        struct DrawData {
            DepthBoundsMode depthBoundsMode;
            graph::FrameGraph& graph;
            flecs::world& world;
            flecs::entity camera;

            flecs::query<
                const ecs::ObjectDeviceData,
                const render::ecs::IndexBuffer,
                const render::ecs::VertexBuffer
            > objectDrawData;

            DrawData(
                DepthBoundsMode depthBoundsMode,
                graph::FrameGraph& graph,
                flecs::world& world,
                flecs::entity camera)
                : depthBoundsMode(depthBoundsMode)
                , graph(graph)
                , world(world)
                , camera(camera)
            {
                init();
            }

        private:
            void init();
        };

        extern flecs::query<
            const world::ecs::Position,
            const world::ecs::Intensity,
            const world::ecs::Colour
        > gAllPointLights;

        extern flecs::query<
            const world::ecs::Position,
            const world::ecs::Direction,
            const world::ecs::Intensity,
            const world::ecs::Colour
        > gAllSpotLights;

        void initSystems(flecs::world& world, render::IDeviceContext &context);

        void copyLightData(
            DrawData& dd,
            graph::Handle& spotLightVolumeData,
            graph::Handle& pointLightVolumeData,
            graph::Handle& spotLightData,
            graph::Handle& pointLightData
        );

        void depthPrePass(
            DrawData& dd,
            graph::Handle& depthTarget
        );

        void lightBinning(
            DrawData& dd,
            graph::Handle& indices,
            graph::Handle depth,
            graph::Handle pointLightData,
            graph::Handle spotLightData
        );

        void forwardPlusOpaque(
            DrawData& dd,
            graph::Handle lightIndices,
            graph::Handle pointLightVolumeData,
            graph::Handle spotLightVolumeData,
            graph::Handle pointLightData,
            graph::Handle spotLightData,
            graph::Handle& target,
            graph::Handle& depth
        );
    }

    /// @brief forward opaque rendering pass
    ///
    /// @param graph the render graph
    /// @param[out] target the render target that will be rendered to
    /// @param[out] depth the depth target that will be rendered to
    /// @param camera the camera to render from
    void opaque(
        graph::FrameGraph& graph,
        graph::Handle& target,
        graph::Handle& depth,
        const Camera& camera
    );

    /// @brief copy a texture to the render target
    ///
    /// @param graph the render graph
    /// @param target the render target to copy to
    /// @param source the texture to copy from
    /// @param viewport the viewport to copy to
    void blit(
        graph::FrameGraph& graph,
        graph::Handle target,
        graph::Handle source,
        const render::Viewport& viewport
    );
}
