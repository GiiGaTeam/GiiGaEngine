#pragma once
#include <pybind11/embed.h>
#include <pybind11/smart_holder.h>
#include <pybind11/stl.h>

#include<directxtk12/SimpleMath.h>


#include<json/json.h>

#include "Planner.h"

using namespace DirectX::SimpleMath;


#include<Component.h>
#include<GameObject.h>
#include<PyBehaviourTrampoline.h>
#include<TransformComponent.h>
#include<Uuid.h>
#include<DXMathUtils.h>
#include<CameraComponent.h>
#include<Logger.h>
#include<Engine.h>
#include<Input.h>
#include<CollisionComponent.h>

PYBIND11_EMBEDDED_MODULE(GiiGaPy, m)
{
    pybind11::class_<GiiGa::RenderSystem, std::shared_ptr<GiiGa::RenderSystem>>(m, "RenderSystem")
        .def("SetCamera", &GiiGa::RenderSystem::SetCamera);

    pybind11::class_<GiiGa::Engine>(m, "Engine")
        .def_static("Instance", []()-> GiiGa::Engine& {
                        return GiiGa::Engine::Instance();
                    },
                    pybind11::return_value_policy::reference)
        .def("RenderSystem", &GiiGa::Engine::RenderSystem, pybind11::return_value_policy::reference_internal);

    pybind11::enum_<GiiGa::MouseButton>(m, "MouseButton")
        .value("Left", GiiGa::MouseButton::MouseLeft)
        .value("Right", GiiGa::MouseButton::MouseRight)
        .export_values();

    pybind11::enum_<GiiGa::KeyCode>(m, "KeyCode")
        .value("KeyW", GiiGa::KeyCode::KeyW)
        .value("KeyA", GiiGa::KeyCode::KeyA)
        .value("KeyS", GiiGa::KeyCode::KeyS)
        .value("KeyD", GiiGa::KeyCode::KeyD)
        .value("KeySpace", GiiGa::KeyCode::KeySpace)
        .export_values();

    pybind11::class_<GiiGa::Input>(m, "Input")
        .def_static("GetMouseDelta", &GiiGa::Input::GetMouseDelta)
        .def_static("IsKeyHeld", [](GiiGa::MouseButton button)
        {
            return GiiGa::Input::IsKeyHeld(button);
        })
        .def_static("IsKeyHeld", [](GiiGa::KeyCode button)
        {
            return GiiGa::Input::IsKeyHeld(button);
        });

    pybind11::class_<GiiGa::Uuid>(m, "Uuid")
        .def(pybind11::init<>())
        .def("__str__", &GiiGa::Uuid::ToString);

    pybind11::class_<Json::Value>(m, "JsonValue")
        .def(pybind11::init<>())
        .def(pybind11::init<int>())
        .def(pybind11::init<double>())
        .def(pybind11::init([](const std::string& s)
        {
            return Json::Value(s);
        }))
        .def_static("FromStyledString", [](const std::string& str)
        {
            Json::Value json;
            Json::Reader reader;
            reader.parse(str, json);
            return json;
        });

    pybind11::class_<Vector3>(m, "Vector3")
        .def(pybind11::init<>())                    // Default constructor
        .def(pybind11::init<float>())               // Single float constructor
        .def(pybind11::init<float, float, float>()) // Three float constructor
        .def(pybind11::init(&GiiGa::Vector3FromJson))
        .def_property("x", [](Vector3* self)
                      {
                          return self->x;
                      }, [](Vector3* self, float v)
                      {
                          self->x = v;
                      })
        .def_property("y", [](Vector3* self)
                      {
                          return self->y;
                      }, [](Vector3* self, float v)
                      {
                          self->y = v;
                      })
        .def_property("z", [](Vector3* self)
                      {
                          return self->z;
                      }, [](Vector3* self, float v)
                      {
                          self->z = v;
                      })
        // Comparison operators using magic methods
        .def("__eq__", &Vector3::operator==)
        .def("__ne__", &Vector3::operator!=)

        // Arithmetic operators using magic methods
        .def("__iadd__", &Vector3::operator+=)
        .def("__add__", [](Vector3* self, Vector3* other)
        {
            return *self + *other;
        })
        .def("__isub__", &Vector3::operator-=)
        .def("__imul__", pybind11::overload_cast<const Vector3&>(&Vector3::operator*=))
        .def("__imul__", pybind11::overload_cast<float>(&Vector3::operator*=))
        .def("MulFloat", [](Vector3* self, float v)
        {
            return *self * v;
        })
        .def("__itruediv__", &Vector3::operator/=)
        .def("__neg__", &Vector3::operator-)

        // Methods
        .def("length", &Vector3::Length)
        .def("length_squared", &Vector3::LengthSquared)
        .def("dot", &Vector3::Dot)
        .def("cross", pybind11::overload_cast<const Vector3&>(&Vector3::Cross, pybind11::const_))
        .def("normalize", pybind11::overload_cast<>(&Vector3::Normalize))
        .def("clamp", pybind11::overload_cast<const Vector3&, const Vector3&>(&Vector3::Clamp))

        // Constants
        .def_readonly_static("Zero", &Vector3::Zero)
        .def_readonly_static("One", &Vector3::One)
        .def_readonly_static("Up", &Vector3::Up)
        .def_readonly_static("Forward", &Vector3::Forward)
        .def_readonly_static("Right", &Vector3::Right);

    m.def("Vector3ToJson", &GiiGa::Vector3ToJson);

    m.def("Vector3FromJson", &GiiGa::Vector3FromJson);

    pybind11::class_<GiiGa::Transform>(m, "Transform")
        .def(pybind11::init<Vector3, Vector3, Vector3>(),
             pybind11::arg_v("loc", Vector3::Zero, "Vector3.Zero"),
             pybind11::arg_v("rot", Vector3::Zero, "Vector3.Zero"),
             pybind11::arg_v("scale", Vector3::One, "Vector3.One"))
        .def(pybind11::init<const GiiGa::Transform&>())
        .def("GetUp", &GiiGa::Transform::GetUp)
        .def("GetForward", &GiiGa::Transform::GetForward)
        .def("GetRight", &GiiGa::Transform::GetRight)
        .def("GetRotation", &GiiGa::Transform::GetRotation)
        .def("SetRotation", &GiiGa::Transform::SetRotation)
        .def("__eq__", &GiiGa::Transform::operator==)
        .def("__ne__", &GiiGa::Transform::operator!=);

    pybind11::class_<GiiGa::SpawnParameters>(m, "SpawnParameters")
        .def(pybind11::init())
        .def_property("name", [](GiiGa::SpawnParameters* self)
                      {
                          return self->name;
                      },
                      [](GiiGa::SpawnParameters* self, const std::string& name)
                      {
                          self->name = name;
                      })
        .def_property("owner", [](GiiGa::SpawnParameters* self)
                      {
                          return self->Owner;
                      },
                      [](GiiGa::SpawnParameters* self, const std::shared_ptr<GiiGa::GameObject>& owner)
                      {
                          self->Owner = owner;
                      });

    pybind11::class_<GiiGa::GameObject, std::shared_ptr<GiiGa::GameObject>>(m, "GameObject")
        .def_readwrite("name", &GiiGa::GameObject::name)
        .def("GetTransformComponent", [](GiiGa::GameObject* self)
        {
            return self->GetTransformComponent().lock();
        }, "Returns TransformComponent type")
        .def("CreateCollisionComponent", &GiiGa::GameObject::CreateComponent<GiiGa::CollisionComponent>)
        .def("Destroy", &GiiGa::GameObject::Destroy)
        .def_static("CreateEmptyGameObject", &GiiGa::GameObject::CreateEmptyGameObject)
        .def("CreateComponent", [](std::shared_ptr<GiiGa::GameObject> self, pybind11::object type)
        {
            auto inst = type();
            auto comp = inst.cast<std::shared_ptr<GiiGa::Component>>();
            comp->RegisterInWorld();
            self->AddComponent(comp);
            return comp;
        },"Argument Any Component subclass type");

    pybind11::classh<GiiGa::Component, GiiGa::PyBehaviourTrampoline>(m, "Component")
        .def(pybind11::init<>())
        .def_property("owner", [](GiiGa::Component* self)
                      {
                          return std::dynamic_pointer_cast<GiiGa::GameObject>(self->GetOwner());
                      },
                      &GiiGa::Component::SetOwner)
        .def("Init", &GiiGa::Component::Init)
        .def("BeginPlay", &GiiGa::Component::BeginPlay)
        .def("Tick", &GiiGa::Component::Tick)
        .def("Destroy", &GiiGa::Component::Destroy)
        .def("RegisterInWorld", [](std::shared_ptr<GiiGa::Component> self)
        {
            self->RegisterInWorld();
        });

    pybind11::class_<GiiGa::TransformComponent, std::shared_ptr<GiiGa::TransformComponent>, GiiGa::Component>(m, "TransformComponent")
        .def(pybind11::init<>())
        .def("GetTransform", &GiiGa::TransformComponent::GetTransform)
        .def("SetTransform", &GiiGa::TransformComponent::SetTransform)
        .def("GetLocation", &GiiGa::TransformComponent::GetLocation)
        .def("SetLocation", &GiiGa::TransformComponent::SetLocation)
        .def("AddLocation", &GiiGa::TransformComponent::AddLocation)
        .def("GetRotation", pybind11::overload_cast<>(&GiiGa::TransformComponent::GetRotation, pybind11::const_))
        .def("SetRotation", pybind11::overload_cast<const Vector3&>(&GiiGa::TransformComponent::SetRotation))
        .def("AddRotation", &GiiGa::TransformComponent::AddRotation)
        .def("GetScale", &GiiGa::TransformComponent::GetScale)
        .def("SetScale", &GiiGa::TransformComponent::SetScale)
        .def("AddScale", &GiiGa::TransformComponent::AddScale)
        .def("GetWorldTransform", &GiiGa::TransformComponent::GetWorldTransform)
        .def("SetWorldTransform", &GiiGa::TransformComponent::SetWorldTransform)
        .def("GetWorldLocation", &GiiGa::TransformComponent::GetWorldLocation)
        .def("SetWorldLocation", &GiiGa::TransformComponent::SetWorldLocation)
        .def("AddWorldLocation", &GiiGa::TransformComponent::AddWorldLocation)
        .def("GetWorldRotation", pybind11::overload_cast<>(&GiiGa::TransformComponent::GetWorldRotation, pybind11::const_))
        .def("SetWorldRotation", pybind11::overload_cast<const Vector3&>(&GiiGa::TransformComponent::SetWorldRotation))
        .def("AddWorldRotation", &GiiGa::TransformComponent::AddWorldRotation)
        .def("GetWorldScale", &GiiGa::TransformComponent::GetWorldScale)
        .def("SetWorldScale", &GiiGa::TransformComponent::SetWorldScale)
        .def("AddWorldScale", &GiiGa::TransformComponent::AddWorldScale)
        .def("GetParent", &GiiGa::TransformComponent::GetParent)
        .def("Forward", [](std::shared_ptr<GiiGa::TransformComponent> self)
        {
            return self->GetWorldMatrix().Forward();
        });

    pybind11::class_<GiiGa::CameraComponent, std::shared_ptr<GiiGa::CameraComponent>, GiiGa::Component>(m, "CameraComponent")
        .def(pybind11::init<>());

    pybind11::class_<GiiGa::CollideInfo>(m, "CollideInfo")
        .def(pybind11::init<>())
        .def_property("baseOffset", [](GiiGa::CollideInfo* self)
                      {
                          return self->baseOffset;
                      },
                      [](GiiGa::CollideInfo* self, const Vector3& baseOffset)
                      {
                          self->baseOffset = baseOffset;
                      })
        .def_property("normal", [](GiiGa::CollideInfo* self)
                      {
                          return self->normal;
                      },
                      [](GiiGa::CollideInfo* self, const Vector3& normal)
                      {
                          self->normal = normal;
                      })
        .def_property("depthPenetration", [](GiiGa::CollideInfo* self)
                      {
                          return self->depthPenetration;
                      },
                      [](GiiGa::CollideInfo* self, float depthPenetration)
                      {
                          self->depthPenetration = depthPenetration;
                      });

    pybind11::class_<GiiGa::ICollision, std::shared_ptr<GiiGa::ICollision>, GiiGa::TransformComponent>(m, "ICollision")
        .def("AddForce", &GiiGa::ICollision::AddForce);

    pybind11::enum_<GiiGa::EMotionType>(m, "EMotionType")
        .value("Dynamic", GiiGa::EMotionType::Dynamic)
        .value("Kinematic", GiiGa::EMotionType::Kinematic)
        .value("Static", GiiGa::EMotionType::Static)
        .export_values();

    pybind11::enum_<GiiGa::Layer>(m, "Layer")
        .value("Moving", GiiGa::Layer::Moving)
        .value("Trigger", GiiGa::Layer::Trigger)
        .value("NoMoving", GiiGa::Layer::NoMoving)
        .export_values();

    pybind11::class_<GiiGa::CollisionComponent, std::shared_ptr<GiiGa::CollisionComponent>, GiiGa::ICollision>(m, "CollisionComponent")
        .def(pybind11::init<>())
        .def("AddForce", &GiiGa::CollisionComponent::AddForce)
        .def("AddImpulse", &GiiGa::CollisionComponent::AddImpulse)
        .def("AddVelocity", &GiiGa::CollisionComponent::AddVelocity)
        .def_property("owner", [](std::shared_ptr<GiiGa::CollisionComponent> self)
                      {
                          return std::dynamic_pointer_cast<GiiGa::GameObject>(self->GetOwner());
                      },
                      &GiiGa::ICollision::SetOwner)
        .def_property("MotionType", &GiiGa::CollisionComponent::GetMotionType, &GiiGa::CollisionComponent::SetMotionType)
        .def_property("Layer", &GiiGa::CollisionComponent::GetLayer, &GiiGa::CollisionComponent::SetLayer);

    pybind11::class_<GiiGa::ShapeCastResult, std::shared_ptr<GiiGa::ShapeCastResult>>(m, "ShapeCastResult")
        .def(pybind11::init<>())
        .def_property("collisionComponent", [](std::shared_ptr<GiiGa::ShapeCastResult> self)
                      {
                          return std::dynamic_pointer_cast<GiiGa::CollisionComponent>(self->collision);
                      },
                      [](GiiGa::ShapeCastResult* self, std::shared_ptr<GiiGa::CollisionComponent> comp)
                      {
                          self->collision = comp;
                      });

    m.def("ShapeCast", [](float sphereRadius, Vector3 startPosition, Vector3 endPosition)
    {
        auto res = GiiGa::PhysicsSystem::ShapeCast(sphereRadius, startPosition, endPosition);
        //el::Loggers::getLogger(GiiGa::LogPyScript)->debug("%v",std::dynamic_pointer_cast<GiiGa::GameObject>(res.collision->GetOwner())->name);
        return res;
    });


    auto goap_m = m.def_submodule("GOAP");

    pybind11::class_<goap::WorldState, std::shared_ptr<goap::WorldState>>(goap_m, "WorldState")
        .def(pybind11::init<>())
        .def(pybind11::init<const std::unordered_map<std::string, bool>&>())
        .def("hasKey", &goap::WorldState::hasKey)
        .def("setValue", &goap::WorldState::setVariable)
        .def("print", &goap::WorldState::Print);

    pybind11::classh<goap::Action>(goap_m, "Action")
        .def(pybind11::init<const std::unordered_map<std::string, bool>&, const std::unordered_map<std::string, bool>&, int>());

    pybind11::class_<goap::Planner>(goap_m, "Planner")
        .def(pybind11::init<>())
        .def("plan", &goap::Planner::plan);
}
