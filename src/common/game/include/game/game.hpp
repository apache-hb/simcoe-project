#pragma once

// #include "meta/class.hpp"
// #include "game/object.hpp"

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

        PhysicsBodyImpl *getImpl() const { return mImpl.get(); }
    };

    struct CharacterBodyImpl;

    void cbiDestroy(CharacterBodyImpl *body);

    class CharacterBody {
        sm::FnUniquePtr<CharacterBodyImpl, cbiDestroy> mImpl;

    public:
        CharacterBody(CharacterBodyImpl *impl)
            : mImpl(impl)
        { }

        math::float3 getUpVector() const;
        void setUpVector(const math::float3& up);

        math::float3 getPosition() const;
        math::quatf getRotation() const;
        void setRotation(const math::quatf& rotation);

        math::float3 getGroundNormal() const;

        void setLinearVelocity(const math::float3& velocity);
        math::float3 getLinearVelocity() const;

        void updateGroundVelocity();
        math::float3 getGroundVelocity() const;

        void update(float dt);
        void postUpdate();

        bool isOnGround() const;
        bool isOnSteepSlope() const;
        bool isInAir() const;
        bool isNotSupported() const;

        bool isSupported() const;

        CharacterBodyImpl *getImpl() const { return mImpl.get(); }
    };

    struct GameContextImpl;

    void gciDestroy(GameContextImpl *impl);

    class Context {
        GameContextImpl *mImpl;

    public:
        Context(GameContextImpl *impl)
            : mImpl(impl)
        { }

        // void addClass(const meta::Class& cls);

        // template<IsObject T>
        // T& spawn() {
        //     return static_cast<T&>(gciCreateObject(mImpl, T::getClass()));
        // }

        // void addObject(game::Object *object);

        // void destroyObject(game::Object *object);

        void tick(float dt);

        void shutdown();

        void setCamera(const draw::Camera& camera);

        math::float3 getGravity() const;

        void debugDrawPhysicsBody(const PhysicsBody& body);

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

        CharacterBody addCharacterBody(
            const world::Cylinder& shape,
            const math::float3& position,
            const math::quatf& rotation,
            bool activate = false);

        world::World& getWorld();
    };

    Context getContext();

    Context init(world::World& world, const draw::Camera& camera);

    void physics_debug(
        graph::FrameGraph& graph,
        const draw::Camera& camera,
        graph::Handle target);
}
