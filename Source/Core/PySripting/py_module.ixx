module;

#include <pybind11/embed.h>
#include <pybind11/smart_holder.h>

#include<directxtk/SimpleMath.h>

export module PyModule;

import <json/json.h>;

using namespace DirectX::SimpleMath;


import Component;
import GameObject;
import PyBehaviourTrampoline;
import TransformComponent;
import Uuid;
import MathUtils;

import Logger;

PYBIND11_EMBEDDED_MODULE(GiiGaPy, m)
{
    pybind11::class_<GiiGa::Uuid>(m, "Uuid")
        .def(pybind11::init<>())
        .def("__str__", &GiiGa::Uuid::ToString);
    
    pybind11::class_<Json::Value>(m, "JsonValue")
        .def(pybind11::init<>())
        .def(pybind11::init<int>())
        .def(pybind11::init<double>())
        .def(pybind11::init([](const std::string& s)
        {
            el::Loggers::getLogger(GiiGa::LogPyScript)->debug("Json from string");
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
        // Comparison operators using magic methods
        .def("__eq__", &Vector3::operator==)
        .def("__ne__", &Vector3::operator!=)

        // Arithmetic operators using magic methods
        .def("__iadd__", &Vector3::operator+=)
        .def("__isub__", &Vector3::operator-=)
        .def("__imul__", pybind11::overload_cast<const Vector3&>(&Vector3::operator*=))
        .def("__imul__", pybind11::overload_cast<float>(&Vector3::operator*=))
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
             pybind11::arg("loc") = Vector3::Zero,
             pybind11::arg("rot") = Vector3::Zero,
             pybind11::arg("scale") = Vector3::One)
        .def(pybind11::init<const GiiGa::Transform&>())
        .def("GetUp", &GiiGa::Transform::GetUp)
        .def("GetForward", &GiiGa::Transform::GetForward)
        .def("GetRight", &GiiGa::Transform::GetRight)
        .def("GetMatrix", &GiiGa::Transform::GetMatrix)
        .def("GetRotation", &GiiGa::Transform::GetRotation)
        .def("SetRotation", &GiiGa::Transform::SetRotation)
        .def("__eq__", &GiiGa::Transform::operator==)
        .def("__ne__", &GiiGa::Transform::operator!=);

    pybind11::class_<GiiGa::GameObject, std::shared_ptr<GiiGa::GameObject>>(m, "GameObject")
        .def_readwrite("name", &GiiGa::GameObject::name)
        .def("GetTransformComponent", [](GiiGa::GameObject* self)
        {
            return self->GetTransformComponent().lock();
        });

    pybind11::classh<GiiGa::Component, GiiGa::PyBehaviourTrampoline>(m, "Component")
        .def(pybind11::init<>())
        .def_property("owner", [](GiiGa::Component* self)
                      {
                          return std::dynamic_pointer_cast<GiiGa::GameObject>(self->GetOwner());
                      },
                      &GiiGa::Component::SetOwner)
        .def("Init", &GiiGa::Component::Init)
        .def("BeginPlay", &GiiGa::Component::BeginPlay)
        .def("Tick", &GiiGa::Component::Tick);

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
        .def("GetWorldMatrix", &GiiGa::TransformComponent::GetWorldMatrix)
        .def("GetInverseWorldMatrix", &GiiGa::TransformComponent::GetInverseWorldMatrix)
        .def("GetLocalMatrix", &GiiGa::TransformComponent::GetLocalMatrix)
        .def("GetInverseLocalMatrix", &GiiGa::TransformComponent::GetInverseLocalMatrix);
}
