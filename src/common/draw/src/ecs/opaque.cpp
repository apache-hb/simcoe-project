#include "stdafx.hpp"

#include "world/ecs.hpp"

#include "draw/draw.hpp"

using namespace sm;

void draw::init_ecs(render::IDeviceContext &context, flecs::world& world) {
    world.observer<const world::ecs::Object>()
        .event(flecs::OnAdd)
        .each([&context](flecs::iter& it, size_t i, const world::ecs::Object& obj) {
            render::ConstBuffer<ObjectData> cbuffer = render::newConstBuffer<ObjectData>(context);

            it.entity(i).set<ecs::ObjectDeviceData>({ std::move(cbuffer) });
        });

    world.observer<const world::ecs::Shape>()
        .event(flecs::OnAdd)
        .each([&context](flecs::iter& it, size_t i, const world::ecs::Shape& shape) {
            flecs::entity entity = it.entity(i);

            world::Mesh mesh = std::visit([](auto it) { return world::primitive(it); }, shape.info);
            world::ecs::AABB aabb = std::visit([](auto it) { return world::ecs::bounds(it); }, shape.info);

            entity.set(context.uploadIndexBuffer(std::move(mesh.indices)));
            entity.set(context.uploadVertexBuffer(std::move(mesh.vertices)));
            entity.set(aabb);

            logs::gGlobal.info("uploading shape data for {}", entity.name().c_str());
        });
}

void draw::opaque_ecs(
    graph::FrameGraph &graph,
    graph::Handle &target,
    graph::Handle &depth,
    flecs::world &ecs)
{

}
