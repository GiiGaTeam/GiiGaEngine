#pragma once
// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.

#include <Jolt/Jolt.h>
#include "easylogging++.h"

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
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>
#include <ICollision.h>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS
// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

#define JPH_DEBUG_RENDERER
#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#define JPH_OBJECT_STREAM
// We're also using STL classes in this example
using namespace std;

namespace GiiGa
{
    class PhysicsSystem;

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
        static constexpr JPH::ObjectLayer TRIGGER = 2;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 3;
    };

    /// Class that determines if two object layers can collide
    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            if (inObject1 == Layers::TRIGGER || inObject2 == Layers::TRIGGER) return true;
            if (inObject1 == Layers::MOVING || inObject2 == Layers::MOVING) return true;
            return false;
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
        static constexpr JPH::BroadPhaseLayer TRIGGER(2);
        static constexpr JPH::uint NUM_LAYERS(3);
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
            mObjectToBroadPhase[Layers::TRIGGER] = BroadPhaseLayers::TRIGGER;
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
            case (BroadPhaseLayer::Type)BroadPhaseLayers::TRIGGER:		return "TRIGGER";
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
            if (inLayer1 == Layers::TRIGGER || inLayer2 == BroadPhaseLayers::TRIGGER) return true;
            if (inLayer1 == Layers::MOVING || inLayer2 == BroadPhaseLayers::MOVING) return true;
            return false;
        }
    };

    // An example activation listener
    class MyBodyActivationListener : public JPH::BodyActivationListener
    {
    public:
        virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
        {
            el::Loggers::getLogger(LogPhysics)->debug("A body got activated");
        }

        virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
        {
            el::Loggers::getLogger(LogPhysics)->debug("A body went to sleep");
        }
    };

    struct ShapeCastResult
    {
        std::shared_ptr<ICollision> collision;
    };


    class CastShapeCollector : public JPH::CastShapeCollector
    {
    public:
        void AddHit(const ResultType& inResult) override
        {
            results.push_back(inResult);
        }

        std::vector<ResultType> results;
    };

    class PhysicsSystem
    {
        // An example contact listener
        class GiiGaContactListener : public JPH::ContactListener
        {
        public:
            virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
            {
                el::Loggers::getLogger(LogPhysics)->debug("Contact validate callback");

                // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
                return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
            }

            virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
            {
                el::Loggers::getLogger(LogPhysics)->debug("A contact was added");
                const auto collision1 = GetCollisionByBody(inBody1.GetID());
                const auto collision2 = GetCollisionByBody(inBody2.GetID());
                CollideInfo collideInfo
                {
                    JoltVecToVec(inManifold.mBaseOffset),
                    JoltVecToVec(inManifold.mWorldSpaceNormal),
                    inManifold.mPenetrationDepth
                };
                if (!collision1 || !collision2) return;
                collision1->OnContactAdded(collision2, collideInfo);
                collision1->OnContactAdded(collision1, collideInfo);
            }

            virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
            {
                //el::Loggers::getLogger(LogPhysics)->debug("A contact was persisted");
                const auto collision1 = GetCollisionByBody(inBody1.GetID());
                const auto collision2 = GetCollisionByBody(inBody2.GetID());
                CollideInfo collideInfo
                {
                    JoltVecToVec(inManifold.mBaseOffset),
                    JoltVecToVec(inManifold.mWorldSpaceNormal),
                    inManifold.mPenetrationDepth
                };
                if (!collision1 || !collision2) return;
                collision1->OnContactPersisted(collision2, collideInfo);
                collision1->OnContactPersisted(collision1, collideInfo);
            }

            virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
            {
                el::Loggers::getLogger(LogPhysics)->debug("A contact was added");
                const auto collision1 = GetCollisionByBody(inSubShapePair.GetBody1ID());
                const auto collision2 = GetCollisionByBody(inSubShapePair.GetBody2ID());

                if (!collision1 || !collision2) return;
                collision1->OnContactRemoved(collision2);
                collision1->OnContactRemoved(collision1);
                el::Loggers::getLogger(LogPhysics)->debug("A contact was removed");
            }
        };

    protected:
        JPH::uint cMaxBodies = 65536;
        JPH::uint cNumBodyMutexes = 0;
        JPH::uint cMaxBodyPairs = 65536;
        JPH::uint cMaxContactConstraints = 10240;

    public:
        static PhysicsSystem& GetInstance()
        {
            if (instance_) return *std::static_pointer_cast<PhysicsSystem>(instance_);
            else return *std::static_pointer_cast<PhysicsSystem>(instance_ = std::shared_ptr<PhysicsSystem>(new PhysicsSystem()));
        }

        void Initialize()
        {
            JPH::RegisterDefaultAllocator();
            temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

            JPH::Trace = TraceImpl;
            JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();

            physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

            physics_system.SetBodyActivationListener(&body_activation_listener);

            physics_system.SetContactListener(&contact_listener);
        }

        static void Simulate(float dt)
        {
            auto& body_interface = GetInstance().physics_system.GetBodyInterface();

            JPH::JobSystemThreadPool job_system(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
            auto& instance = GetInstance();
            for (auto [uuid, body] : instance.collision_body_map_)
            {
                const auto col_comp = WorldQuery::GetWithUUID<ICollision>(uuid);
                if (!col_comp) continue;

                JPH::RVec3 position = body_interface.GetCenterOfMassPosition(body);
                JPH::Quat rotation = body_interface.GetRotation(body);
                if (body_interface.IsActive(body))
                    col_comp->SetOwnerWorldLocation(JoltVecToVec(position));
                if (body_interface.IsActive(body))
                    col_comp->SetOwnerWorldRotation(JoltQuatToQuat(rotation));
            }

            const int cCollisionSteps = 1;

            instance.physics_system.Update(dt, cCollisionSteps, instance.temp_allocator, &job_system);
        }

        void BindOnTransformUpdate(const std::shared_ptr<ICollision>& collision_comp)
        {
            if (const auto& owner_trans = std::dynamic_pointer_cast<GameObject>(collision_comp->GetOwner())->GetTransformComponent().lock())
            {
                update_transform_cashed_.emplace(collision_comp->GetUuid(),
                                                 owner_trans->OnUpdateTransform.Register([this](const std::shared_ptr<TransformComponent>& trans)
                                                 {
                                                     if (const auto& owner = std::dynamic_pointer_cast<GameObject>(trans->GetOwner()))
                                                     {
                                                         const auto& col_comp = owner->GetComponent<ICollision>();
                                                         const auto& bodyID = GetInstance().collision_body_map_.at(col_comp->GetUuid());
                                                         GetBodyInterface().SetPositionAndRotation(bodyID,
                                                                                                   VecToJoltVec(trans->GetWorldLocation()),
                                                                                                   QuatToJoltQuat(trans->GetWorldQuatRotation()),
                                                                                                   JPH::EActivation::Activate);
                                                     }
                                                 }));
            }
        }

        static JPH::BodyID RegisterCollision(const std::shared_ptr<ICollision>& collision_comp)
        {
            auto& body_interface = GetInstance().physics_system.GetBodyInterface();

            JPH::BodyID body{};
            auto& instance = GetInstance();
            if (!collision_comp) return body;

            DirectX::SimpleMath::Vector3 range = collision_comp->GetWorldScale();
            Transform trans = collision_comp->GetWorldTransform();

            const auto Layer = collision_comp->GetLayer();
            switch (collision_comp->GetColliderType())
            {
            case ColliderType::Cube:
            {
                JPH::BoxShapeSettings box_shape_settings(VecToJoltVec(range));
                box_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

                JPH::ShapeSettings::ShapeResult box_shape_result = box_shape_settings.Create();
                if (box_shape_result.HasError())
                {
                    //log(floor_shape_result.GetError());
                    return body;
                }
                JPH::ShapeRefC box_shape = box_shape_result.Get();

                //TODO Испрвить Слои - пока все moving
                trans = collision_comp->GetWorldTransform();
                JPH::BodyCreationSettings box_settings(box_shape, VecToJoltVec(trans.location_), QuatToJoltQuat(trans.rotate_), static_cast<JPH::EMotionType>(collision_comp->GetMotionType()), Layer);
                box_settings.mIsSensor = Layer == Trigger;
                body = body_interface.CreateBody(box_settings)->GetID();
                break;
            }
            case ColliderType::Sphere:
            {
                JPH::SphereShapeSettings sphere_shape_settings(range.x);
                sphere_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

                JPH::ShapeSettings::ShapeResult sphere_shape_result = sphere_shape_settings.Create();
                if (sphere_shape_result.HasError())
                {
                    //log(floor_shape_result.GetError());
                    return body;
                }
                JPH::ShapeRefC sphere_shape = sphere_shape_result.Get();

                //TODO Испрвить Слои - пока все moving

                JPH::BodyCreationSettings sphere_settings(sphere_shape, VecToJoltVec(trans.location_), QuatToJoltQuat(trans.rotate_), static_cast<JPH::EMotionType>(collision_comp->GetMotionType()), Layer);
                sphere_settings.mIsSensor = Layer == Trigger;
                body = body_interface.CreateBody(sphere_settings)->GetID();
                break;
            }
            }

            body_interface.AddBody(body, JPH::EActivation::Activate);
            instance.collision_body_map_.emplace(collision_comp->GetUuid(), body);
            instance.body_collision_map_.emplace(body, collision_comp->GetUuid());
            GetInstance().BindOnTransformUpdate(collision_comp);

            if (collision_comp->IsGravityActive())
            {
                body_interface.SetGravityFactor(body, 1.0);
                body_interface.AddForce(body, {0.0f, 9.8f, 0.0f});
            }
            if (!collision_comp->IsGravityActive())
            {
                body_interface.SetGravityFactor(body, 0.0);
            }

            return body;
        }

        static void UnRegisterCollision(const ICollision* collision_comp)
        {
            if (GetInstance().collision_body_map_.contains(collision_comp->GetUuid()))
            {
                GetInstance().DestroyBody(GetInstance().collision_body_map_.at(collision_comp->GetUuid()));
                GetInstance().update_transform_cashed_.erase(collision_comp->GetUuid());
                GetInstance().body_collision_map_.erase(GetInstance().collision_body_map_.at(collision_comp->GetUuid()));
                GetInstance().collision_body_map_.erase(collision_comp->GetUuid());
            }
        }

        static std::shared_ptr<ShapeCastResult> ShapeCast(float sphereRadius, Vector3 startPositionD, Vector3 endPositionD)
        {
            std::shared_ptr<ShapeCastResult> res{};
            JPH::SphereShape sphereShape(sphereRadius);

            JPH::Vec3 startPosition = VecToJoltVec(startPositionD);
            JPH::Vec3 endPosition = VecToJoltVec(endPositionD);
            JPH::Vec3 sweepDirection = (endPosition - startPosition).Normalized();
            float maxDistance = (endPosition - startPosition).Length();

            // Define motion
            JPH::RShapeCast shapeCast(&sphereShape, JPH::Vec3(1, 1, 1), JPH::Mat44::sIdentity(), sweepDirection * maxDistance);

            // Prepare for the result
            JPH::ShapeCastResult shapeCastResult;

            JPH::ShapeCastSettings settings{};

            CastShapeCollector collector{};

            // Perform the sphere trace
            GetInstance().physics_system.GetNarrowPhaseQuery().CastShape(shapeCast, settings, startPosition, collector,
                                                                         JPH::DefaultBroadPhaseLayerFilter{GetInstance().object_vs_broadphase_layer_filter, Layers::MOVING});
            if (!collector.results.empty())
            {
                res = std::make_shared<ShapeCastResult>();
                res->collision = GetInstance().GetCollisionByBody(collector.results[0].mBodyID2);
            }

            return res;
        }

        static void BeginPlay()
        {
            GetInstance().physics_system.OptimizeBroadPhase();
        }

        static void EndPlay()
        {
            GetInstance().FreshObjects();
        }

        void DestroyBody(const JPH::BodyID& bodyID) const
        {
            GetBodyInterface().RemoveBody(bodyID);
            GetBodyInterface().DeactivateBody(bodyID);
        }

        void FreshObjects()
        {
            for (auto [uuid, body] : collision_body_map_)
            {
                DestroyBody(body);
                WorldQuery::GetWithUUID<ICollision>(uuid)->OnUpdateTransform.Unregister(update_transform_cashed_.at(uuid));
            }
            update_transform_cashed_.clear();
            collision_body_map_.clear();
            body_collision_map_.clear();
        }

        static std::shared_ptr<ICollision> GetCollisionByBody(const JPH::BodyID& inBodyID)
        {
            if (!GetInstance().body_collision_map_.contains(inBodyID)) return nullptr;
            return WorldQuery::GetWithUUID<ICollision>(GetInstance().body_collision_map_.at(inBodyID));
        }

        void Destroy()
        {
            FreshObjects();

            JPH::UnregisterTypes();

            if (temp_allocator) delete temp_allocator;
            temp_allocator = nullptr;
            if (JPH::Factory::sInstance) delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }

        static JPH::PhysicsSystem& GetJoltPhysicsSystem() { return GetInstance().physics_system; }
        static JPH::BodyInterface& GetBodyInterface() { return GetInstance().physics_system.GetBodyInterface(); }

    protected:
        std::unordered_map<Uuid, JPH::BodyID> collision_body_map_;
        std::unordered_map<JPH::BodyID, Uuid> body_collision_map_;

        std::unordered_map<Uuid, EventHandle<std::shared_ptr<TransformComponent>>> update_transform_cashed_;

        JPH::PhysicsSystem physics_system;
        BPLayerInterfaceImpl broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        ObjectLayerPairFilterImpl object_vs_object_layer_filter;
        MyBodyActivationListener body_activation_listener;
        GiiGaContactListener contact_listener;
        JPH::TempAllocatorImpl* temp_allocator = nullptr;

        static inline std::shared_ptr<PhysicsSystem> instance_;

    private:
        PhysicsSystem()
        {
        };
    };
}
