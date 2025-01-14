module;

#include <pybind11/embed.h>

export module PyProperty;

namespace GiiGa
{
export struct PyProperty
{
    pybind11::type script_type;
    pybind11::object value_or_holder;
};
}