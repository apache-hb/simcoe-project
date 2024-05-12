#pragma once

#include "draw/common.hpp"

#include "render/graph.hpp"
#include "render/render.hpp"
#include "world/ecs.hpp"

namespace sm::draw {
    class Camera;

    static constexpr inline size_t kMaxViewports = MAX_VIEWPORTS;
    static constexpr inline size_t kMaxObjects = MAX_OBJECTS;
    static constexpr inline size_t kMaxLights = MAX_LIGHTS;
    static constexpr inline size_t kMaxPointLights = MAX_POINT_LIGHTS;
    static constexpr inline size_t kMaxSpotLights = MAX_SPOT_LIGHTS;

    template<typename T>
    struct ResourceHandle {
        size_t index = SIZE_MAX;
    };

    static constexpr inline D3D12MA::ALLOCATION_DESC kMemoryAlloc = {
        .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
        .HeapType = D3D12_HEAP_TYPE_UPLOAD,
    };

    template<StandardLayout T>
    class MappedResource {
        render::BufferResource mResource;
        T *mMemory;

    public:
        MappedResource(render::IDeviceContext& context, size_t capacity) noexcept
            : mResource(context, CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * capacity), kMemoryAlloc, D3D12_RESOURCE_STATE_COMMON)
            , mMemory(mResource.mapWriteOnly<T>())
        { }

        void update(size_t index, const T& data) noexcept {
            mMemory[index] = data;
        }

        D3D12_GPU_VIRTUAL_ADDRESS getBaseAddress() const noexcept {
            return mResource.getDeviceAddress();
        }

        D3D12_GPU_VIRTUAL_ADDRESS getAddress(size_t index) const noexcept {
            return getBaseAddress() + sizeof(T) * index;
        }
    };

    template<StandardLayout T>
    class ResourcePool {
        BitMapIndexAllocator mAllocator;
        MappedResource<T> mResource;

    public:
        using Handle = ResourceHandle<T>;

        ResourcePool(render::IDeviceContext& context, size_t capacity) noexcept
            : mAllocator(capacity)
            , mResource(context, capacity)
        { }

        void update(Handle handle, const T& data) noexcept {
            mResource.update(handle.index, data);
        }

        D3D12_GPU_VIRTUAL_ADDRESS getBaseAddress() const noexcept {
            return mResource.getDeviceAddress();
        }

        D3D12_GPU_VIRTUAL_ADDRESS getAddress(Handle handle) const noexcept {
            return mResource.getAddress(handle.index);
        }

        Handle create() noexcept {
            return {mAllocator.allocateIndex()};
        }

        void release(Handle handle) noexcept {
            mAllocator.deallocate(handle.index);
        }
    };

    using ObjectHandle = ResourceHandle<ObjectData>;
    using ViewportHandle = ResourceHandle<ViewportData>;
    using SpotLightHandle = ResourceHandle<SpotLightData>;
    using PointLightHandle = ResourceHandle<PointLightData>;

    class LightPool {
        MappedResource<LightVolumeData> mLightVolumes;
        ResourcePool<PointLightData> mPointLights;
        ResourcePool<SpotLightData> mSpotLights;

        static_assert(kMaxLights == kMaxPointLights + kMaxSpotLights, "light pool sizes do not match");

        size_t computeVolumeIndex(SpotLightHandle handle) const noexcept {
            return kMaxPointLights + handle.index;
        }

        size_t computeVolumeIndex(PointLightHandle handle) const noexcept {
            return handle.index;
        }

    public:
        LightPool(render::IDeviceContext& context) noexcept
            : mLightVolumes(context, kMaxLights)
            , mPointLights(context, kMaxPointLights)
            , mSpotLights(context, kMaxSpotLights)
        { }

        SpotLightHandle createSpotLight() noexcept {
            return mSpotLights.create();
        }

        PointLightHandle createPointLight() noexcept {
            return mPointLights.create();
        }

        void releaseSpotLight(SpotLightHandle handle) noexcept {
            mSpotLights.release(handle);
        }

        void releasePointLight(PointLightHandle handle) noexcept {
            mPointLights.release(handle);
        }

        void updateSpotLight(
            SpotLightHandle handle,
            const LightVolumeData& volume,
            const SpotLightData& data
        ) noexcept {
            mLightVolumes.update(computeVolumeIndex(handle), volume);
            mSpotLights.update(handle, data);
        }

        void updatePointLight(
            PointLightHandle handle,
            const LightVolumeData& volume,
            const PointLightData& data
        ) noexcept {
            mLightVolumes.update(computeVolumeIndex(handle), volume);
            mPointLights.update(handle, data);
        }
    };

    struct WorldDrawData {
        ResourcePool<ObjectData> objects;
        ResourcePool<ViewportData> viewports;
        LightPool lights;

        ViewportHandle activeViewport;

        WorldDrawData(render::IDeviceContext& context)
            : objects(context, kMaxObjects)
            , viewports(context, kMaxViewports)
            , lights(context)
        { }
    };

    enum class DepthBoundsMode {
        eDisabled = DEPTH_BOUNDS_DISABLED,
        eEnabled = DEPTH_BOUNDS_ENABLED,
        eEnabledMSAA = DEPTH_BOUNDS_MSAA,
    };

    enum class AlphaTestMode {
        eDisabled = ALPHA_TEST_DISABLED,
        eEnabled = ALPHA_TEST_ENABLED,
    };

    struct ForwardPlusConfig {
        DepthBoundsMode depthBounds;
        AlphaTestMode alphaTest;
    };

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
