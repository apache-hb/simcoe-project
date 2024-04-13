#pragma once

#include "meta/class.hpp"
#include "game/object.hpp"

#include "world/world.hpp"

#include "render/graph.hpp"

namespace sm::draw {
    class Camera;
}

namespace sm::game {
    class Entity;

    struct PhysicsBodyImpl;

    void pbiDestroy(PhysicsBodyImpl *body);

    class PhysicsBody {
        sm::FnUniquePtr<PhysicsBodyImpl, pbiDestroy> mImpl;

    public:
        PhysicsBody(PhysicsBodyImpl *impl)
            : mImpl(impl)
        { }

        void setLinearVelocity(const math::float3& velocity);
        void setAngularVelocity(const math::float3& velocity);

        void activate();

        bool isActive() const;

        math::float3 getLinearVelocity() const;
        math::float3 getCenterOfMass() const;
        math::quatf getRotation() const;
    };

    struct GameContextImpl;

    void gciDestroy(GameContextImpl *impl);

    PhysicsBody gciAddPhysicsBody(
        GameContextImpl *impl,
        const math::float3& position,
        const math::quatf& rotation,
        bool dynamic);

    class Context {
        GameContextImpl *mImpl;

    public:
        Context(GameContextImpl *impl)
            : mImpl(impl)
        { }

        void addClass(const meta::Class& cls);

        template<IsObject T>
        T& spawn() {
            return static_cast<T&>(gciCreateObject(mImpl, T::getClass()));
        }

        void addObject(game::Object *object);

        void destroyObject(game::Object& object);

        void tick(float dt);

        void shutdown();

        void setCamera(const draw::Camera& camera);

        PhysicsBody addPhysicsBody(
            const world::Cube& cube,
            const math::float3& position,
            const math::quatf& rotation,
            bool dynamic = false);

        PhysicsBody addPhysicsBody(
            const world::Sphere& sphere,
            const math::float3& position,
            const math::quatf& rotation,
            bool dynamic = false);
    };

    Context getContext();

    Context init(world::World& world, const draw::Camera& camera);

    void physics_debug(
        graph::FrameGraph& graph,
        const draw::Camera& camera,
        graph::Handle target);
}
