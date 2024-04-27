#include "stdafx.hpp"

#include "draw/draw.hpp"
#include "world/ecs.hpp"

using namespace sm;

void draw::ecs::initObjectObservers(flecs::world& world, render::IDeviceContext &context) {

    // when an object is added to the world, create the required device data
    // to draw it
    world.observer<const world::ecs::Object>()
        .event(flecs::OnAdd)
        .each([&context](flecs::iter& it, size_t i, const world::ecs::Object& obj) {
            render::ConstBuffer<ObjectData> cbuffer = render::newConstBuffer<ObjectData>(context);

            it.entity(i).set<ecs::ObjectDeviceData>({ std::move(cbuffer) });
        });

    // when a shape is setup, upload its mesh data to the gpu
    world.observer<const world::ecs::Shape>()
        .event(flecs::OnSet)
        .each([&context](flecs::iter& it, size_t i, const world::ecs::Shape& shape) {
            flecs::entity entity = it.entity(i);

            world::Mesh mesh = std::visit([](auto it) { return world::primitive(it); }, shape.info);
            world::ecs::AABB aabb = std::visit([](auto it) { return world::ecs::bounds(it); }, shape.info);

            // TODO: find a way to batch these
            context.upload([&] {
                entity.set(context.uploadIndexBuffer(std::move(mesh.indices)));
                entity.set(context.uploadVertexBuffer(std::move(mesh.vertices)));
                entity.set(aabb);
            });
        });

    // when a camera is added to the world, create the required device data
    // for its viewport
    world.observer<const world::ecs::Camera>()
        .event(flecs::OnAdd)
        .each([&context](flecs::iter& it, size_t i, const world::ecs::Camera& perspective) {
            render::ConstBuffer<ViewportData> cbuffer = render::newConstBuffer<ViewportData>(context);

            it.entity(i).set<ecs::ViewportDeviceData>({ std::move(cbuffer) });
        });

    world.system<
        ecs::ObjectDeviceData,
        const world::ecs::Position,
        const world::ecs::Rotation,
        const world::ecs::Scale>("Update object draw data with new transform")
        .kind(flecs::OnStore)
        .term_at(2).second<world::ecs::World>()
        .iter(
            [](
                flecs::iter& it,
                ecs::ObjectDeviceData *data,
                const world::ecs::Position *pos,
                const world::ecs::Rotation *rot,
                const world::ecs::Scale *scale
            ) {
            for (auto i : it) {
                // TODO: multiplying scale by -1 is a hack, not sure how to fix this
                float4x4 model = math::float4x4::transform(pos[i].position, rot[i].rotation, -scale[i].scale);

                data[i].update({ model });
            }
        });

    world.system<
        ecs::ViewportDeviceData,
        const world::ecs::Camera,
        const world::ecs::Position,
        const world::ecs::Direction>("Update viewport data with new camera info")
        .kind(flecs::OnStore)
        .term_at(3).second<world::ecs::World>()
        .iter(
            [](
                flecs::iter& it,
                ecs::ViewportDeviceData *data,
                const world::ecs::Camera *camera,
                const world::ecs::Position *pos,
                const world::ecs::Direction *dir
            ) {
            for (auto i : it) {
                float4x4 v = world::ecs::getViewMatrix(pos[i], dir[i]);
                float4x4 p = camera[i].getProjectionMatrix();

                data[i].update(draw::ViewportData {
                    .viewProjection = float4x4::identity(),
                    .worldView = v.transpose(),
                    .projection = p.transpose(),
                });
            }
        });
}