#pragma once
// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#define JPH_DEBUG_RENDERER
#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#define JPH_OBJECT_STREAM
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>

#include "CollisionComponent.h"

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// We're also using STL classes in this example
using namespace std;

namespace GiiGa
{
    // Callback for traces, connect this to your own trace function if you have one
    static void TraceImpl(const char* inFMT, ...)
    {
        // Format the message
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);

        // Print to the TTY
        cout << buffer << endl;
    }

#ifdef JPH_ENABLE_ASSERTS

    // Callback for asserts, connect this to your own assert handler if you have one
    static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
    {
        // Print to the TTY
        cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << endl;

        // Breakpoint
        return true;
    };

#endif // JPH_ENABLE_ASSERTS

    // Layer that objects can be in, determines which other objects it can collide with
    // Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
    // layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
    // but only if you do collision testing).
    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    };

    /// Class that determines if two object layers can collide
    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    // Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
    // a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
    // You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
    // many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
    // your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr JPH::uint NUM_LAYERS(2);
    };

    // BroadPhaseLayerInterface implementation
    // This defines a mapping between object and broadphase layers.
    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            // Create a mapping table from object to broad phase layer
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual JPH::uint GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            //JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char *			GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
        {
            switch ((BroadPhaseLayer::Type)inLayer)
            {
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
            default:													JPH_ASSERT(false); return "INVALID";
            }
        }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

    private:
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    /// Class that determines if an object layer can collide with a broadphase layer
    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    // An example contact listener
    class GiiGaContactListener : public JPH::ContactListener
    {
    public:
        // See: ContactListener
        virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
        {
            cout << "Contact validate callback" << endl;

            // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
        {
            cout << "A contact was added" << endl;
        }

        virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
        {
            cout << "A contact was persisted" << endl;
        }

        virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
        {
            cout << "A contact was removed" << endl;
        }
    };

    // An example activation listener
    class MyBodyActivationListener : public JPH::BodyActivationListener
    {
    public:
        virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
        {
            cout << "A body got activated" << endl;
        }

        virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
        {
            cout << "A body went to sleep" << endl;
        }
    };

    // Program entry point
    class PhysicsSystem
    {
    protected:
        constexpr JPH::uint cMaxBodies = 65536;
        constexpr JPH::uint cNumBodyMutexes = 0;
        constexpr JPH::uint cMaxBodyPairs = 65536;
        constexpr JPH::uint cMaxContactConstraints = 10240;
        static inline std::shared_ptr<PhysicsSystem> instance_;
        PhysicsSystem() = default;

    public:
        static PhysicsSystem& GetInstance()
        {
            if (instance_)
                return *instance_;
            else
            {
                el::Loggers::getLogger(LogWorld)->fatal("Failed to get instance of WorldQuery, Call To World Init first!");
                throw std::runtime_error("Failed to get instance of WorldQuery, Call To World Init first!");
            }
        }

        virtual ~PhysicsSystem() = default;

        virtual void Initialize()
        {
            JPH::RegisterDefaultAllocator();

            JPH::Trace = TraceImpl;
            JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();

            auto temp_job_sys = JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
            job_system_ = std::shared_ptr<JPH::JobSystemThreadPool>(&temp_job_sys);

            physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

            physics_system.SetBodyActivationListener(&body_activation_listener);

            physics_system.SetContactListener(&contact_listener);

            body_interface = std::shared_ptr<JPH::BodyInterface>(&physics_system.GetBodyInterface());
        }

        virtual void Simulate(float dt)
        {
        }

        void RegisterCollision(std::shared_ptr<CollisionComponent> collision_comp)
        {
            if (!collision_comp) return;

            switch (collision_comp->GetColliderType())
            {
            case ColliderType::Box:
                DirectX::SimpleMath::Vector3 half_extend = collision_comp->GetScale() / 2;
                JPH::BoxShapeSettings box_shape_settings(VecToJoltVec(half_extend));
                box_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

            // Create the shape
                JPH::ShapeSettings::ShapeResult box_shape_result = box_shape_settings.Create();
                if (box_shape_result.HasError())
                {
                    //log(floor_shape_result.GetError());
                    return;
                }
                JPH::ShapeRefC box_shape = box_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

            // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
                JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0_r, -1.0_r, 0.0_r), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);

            // Create the actual rigid body
                JPH::Body* floor = body_interface->CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr
                break;
            case ColliderType::Sphere:
                break;
            };
        }

        std::unordered_map<>
        virtual void BeginPlay();

        virtual void FreshObjects()
        {
            
        }

        virtual void Destroy()
        {
            // Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
            body_interface->RemoveBody(sphere_id);

            // Destroy the sphere. After this the sphere ID is no longer valid.
            body_interface->DestroyBody(sphere_id);

            // Remove and destroy the floor
            body_interface->RemoveBody(floor->GetID());
            body_interface->DestroyBody(floor->GetID());

            // Unregisters all types with the factory and cleans up the default material
            JPH::UnregisterTypes();

            // Destroy the factory
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }

        int main(int argc, char** argv)
        {
            // Next we can create a rigid body to serve as the floor, we make a large box
            // Create the settings for the collision volume (the shape).
            // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
            JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
            floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

            // Create the shape
            JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
            JPH::ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

            // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
            JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0_r, -1.0_r, 0.0_r), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);

            // Create the actual rigid body
            JPH::Body* floor = body_interface->CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

            // Add it to the world
            body_interface->AddBody(floor->GetID(), JPH::EActivation::DontActivate);

            // Now create a dynamic body to bounce on the floor
            // Note that this uses the shorthand version of creating and adding a body to the world
            JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::RVec3(0.0_r, 2.0_r, 0.0_r), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
            JPH::BodyID sphere_id = body_interface->CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

            // Now you can interact with the dynamic body, in this case we're going to give it a velocity.
            // (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
            body_interface->SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, 50.0f, 0.0f));
            body_interface->SetGravityFactor(sphere_id, 9.8f);

            // We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
            const float cDeltaTime = 1.0f / 60.0f;

            // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
            // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
            // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
            physics_system.OptimizeBroadPhase();

            // Now we're ready to simulate the body, keep simulating until it goes to sleep
            JPH::uint step = 0;
            while (body_interface->IsActive(sphere_id))
            {
                // Next step
                ++step;

                // Output current position and velocity of the sphere
                JPH::RVec3 position = body_interface->GetCenterOfMassPosition(sphere_id);
                JPH::Vec3 velocity = body_interface->GetLinearVelocity(sphere_id);
                cout << "Step " << step << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;

                // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
                const int cCollisionSteps = 1;

                // Step the world
                physics_system.Update(cDeltaTime, cCollisionSteps, &temp_allocator, &job_system_);
            }


            return 0;
        }

    protected:
        JPH::PhysicsSystem physics_system;
        BPLayerInterfaceImpl broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        ObjectLayerPairFilterImpl object_vs_object_layer_filter;
        MyBodyActivationListener body_activation_listener;
        GiiGaContactListener contact_listener;
        std::shared_ptr<JPH::BodyInterface> body_interface;
        std::shared_ptr<JPH::JobSystemThreadPool> job_system_;
        JPH::TempAllocatorImpl temp_allocator = JPH::TempAllocatorImpl{10 * 1024 * 1024};
    };
}
