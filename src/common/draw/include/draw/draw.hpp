#pragma once

#include "render/graph.hpp"
#include "render/render.hpp"

namespace sm::draw {
    class Camera;

    ///
    /// forward+ pipeline
    ///

    namespace forward_plus {
        enum class DepthBoundsMode {
            eDisabled = 0,
            eEnabled = 1,
            eEnabledMSAA = 2,
        };

        struct DrawData {
            const Camera& camera;
            DepthBoundsMode depth_mode;

            DrawData(const Camera& camera, DepthBoundsMode mode)
                : camera(camera)
                , depth_mode(mode)
            { }
        };

        /// @brief upload light data to the gpu
        ///
        /// @param graph the render graph
        /// @param[out] point_light_data the handle to the point light data
        /// @param[out] spot_light_data the handle to the spot light data
        void upload_light_data(
            graph::FrameGraph& graph,
            graph::Handle& point_light_data,
            graph::Handle& spot_light_data);

        /// @brief forward+ depth prepass
        ///
        /// @param graph the render graph
        /// @param depth_target the depth target that will be rendered to
        /// @param dd the draw data
        void depth_prepass(
            graph::FrameGraph& graph,
            graph::Handle& depth_target,
            DrawData dd);

        /// @brief forward+ light binning pass
        ///
        /// @param graph the render graph
        /// @param[out] indices the uav that light indices will be written to
        /// @param depth the depth target that will be read from
        /// @param point_light_data the point light data
        /// @param spot_light_data the spot light data
        /// @param dd the draw data
        void light_binning(
            graph::FrameGraph& graph,
            graph::Handle& indices,
            graph::Handle depth,
            graph::Handle point_light_data,
            graph::Handle spot_light_data,
            DrawData dd);

        /// @brief forward+ opaque rendering pass
        /// @pre @a camera must be the same camera used in the forward_cull pass
        ///
        /// @param graph the render graph
        /// @param[out] target the render target that will be rendered to
        /// @param indices the uav that light indices will be read from
        /// @param dd the draw data
        void opaque(
            graph::FrameGraph& graph,
            graph::Handle& target,
            graph::Handle indices,
            DrawData dd);
    }

    ///
    /// forward pipeline
    ///

    /// @brief forward opaque rendering pass
    ///
    /// @param graph the render graph
    /// @param[out] target the render target that will be rendered to
    /// @param camera the camera to render from
    void opaque(
        graph::FrameGraph& graph,
        graph::Handle& target,
        const Camera& camera);


    ///
    /// other passes
    ///

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
        const render::Viewport& viewport);
}
