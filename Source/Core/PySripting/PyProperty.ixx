module;

#include <pybind11/pybind11.h>

export module PyProperty;

namespace GiiGa
{
export struct PyProperty
{
    pybind11::type type;
    pybind11::object value;
};
}