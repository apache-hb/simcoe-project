#pragma once

#include "draw/forward_plus.hpp"

#include "render/graph.hpp"
#include "render/render.hpp"
#include "world/ecs.hpp"

#include "draw/camera.hpp"

LOG_MESSAGE_CATEGORY(DrawLog, "Draw");

namespace sm::draw {
    class Camera;

    namespace ecs {
        using ObjectDeviceData = render::ConstBuffer<ObjectData>;
        using ViewportDeviceData = render::ConstBuffer<ViewportData>;
    }

    namespace host {
        struct ModelData {
            float4x4 model;
            render::ecs::VertexBuffer vbo;
            render::ecs::IndexBuffer ibo;
            ecs::ObjectDeviceData data;
        };

        struct CameraData {
            std::string name;
            world::ecs::Camera camera;
            float3 position;
            float3 direction;

            CameraData(flecs::entity entity)
                : CameraData(entity.name(), *entity.get<world::ecs::Camera>(), *entity.get<world::ecs::Position>(), *entity.get<world::ecs::Direction>())
            { }

            void updateViewportData(ecs::ViewportDeviceData& data, uint pointLightCount, uint spotLightCount) const noexcept;

        private:
            CameraData(flecs::string_view name, const world::ecs::Camera& camera, world::ecs::Position position, world::ecs::Direction direction)
                : name(std::move(name))
                , camera(camera)
                , position(position.position)
                , direction(direction.direction)
            { }
        };

        struct PointLightData {
            float3 position;
            float intensity;
            float3 colour;
        };

        struct SpotLightData {
            float3 position;
            float3 direction;
            float intensity;
            float3 colour;
        };
    }

    namespace ecs {
        struct WorldData {
            flecs::world& world;
            flecs::entity camera;

            flecs::query<
                const ecs::ObjectDeviceData,
                const render::ecs::IndexBuffer,
                const render::ecs::VertexBuffer
            > objectDrawData = world.query<
                const ecs::ObjectDeviceData,
                const render::ecs::IndexBuffer,
                const render::ecs::VertexBuffer
            >();
        };

        struct DrawData {
            DepthBoundsMode depthBoundsMode;
            graph::FrameGraph& graph;
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
            WorldData& wd,
            DrawData& dd,
            graph::Handle& depthTarget
        );

        void lightBinning(
            WorldData& wd,
            DrawData& dd,
            graph::Handle& indices,
            graph::Handle depth,
            graph::Handle pointLightData,
            graph::Handle spotLightData
        );

        void forwardPlusOpaque(
            WorldData& wd,
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
