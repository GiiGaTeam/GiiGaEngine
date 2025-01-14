module;

#include <pybind11/embed.h>

export module ScriptHelpers;

namespace GiiGa
{
    namespace ScriptHelpers
    {
        export std::string GetComponentSubclassNameInModule(const pybind11::module& m)
        {
            pybind11::module_ helpers_module = pybind11::module_::import("GiiGaPyHelpers");
            pybind11::module_ engine_module = pybind11::module_::import("GiiGaPy");
            return helpers_module.attr("get_subclass_name_in_module")(m, engine_module.attr("Component")).cast<std::string>();
        }

        export bool IsEqOrSubClass(const pybind11::type& type, const pybind11::type& base)
        {
            pybind11::module_ helpers_module = pybind11::module_::import("GiiGaPyHelpers");
            return helpers_module.attr("IsEqOrSubClass")(type, base).cast<bool>();
        }
    }
}
