module;

#include <pybind11/embed.h>
#include <pybind11/smart_holder.h>

export module PyModule;


import Component;
import GameObject;
import PyBehaviourTrampoline;

PYBIND11_EMBEDDED_MODULE(GiiGaPy, m)
{
    pybind11::class_<GiiGa::GameObject, std::shared_ptr<GiiGa::GameObject>>(m, "GameObject")
        .def_readwrite("name", &GiiGa::GameObject::name);

    pybind11::classh<GiiGa::Component, GiiGa::PyBehaviourTrampoline>(m, "Component")
        .def(pybind11::init<>())
        .def_property("owner", [](GiiGa::Component* self)
                      {
                          return std::dynamic_pointer_cast<GiiGa::GameObject>(self->GetOwner());
                      },
                      &GiiGa::Component::SetOwner)
        .def("Init", &GiiGa::Component::Init)
        .def("Tick", &GiiGa::Component::Tick);
}
