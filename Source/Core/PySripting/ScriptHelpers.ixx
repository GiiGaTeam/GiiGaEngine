module;

#include <pybind11/embed.h>

export module ScriptHelpers;

import <json/value.h>;

import Logger;

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

        export bool TypeIsSubClassComponent(const pybind11::type& type)
        {
            return IsEqOrSubClass(type,pybind11::module::import("GiiGaPy").attr("Component"));
        }

        export Json::Value EncodeToJSONValue(const pybind11::object& obj)
        {
            pybind11::module_ helpers_module = pybind11::module_::import("GiiGaPyHelpers");
            try
            {
                pybind11::object t = helpers_module.attr("EncodeToJSONValue")(obj);
                return t.cast<Json::Value>();
            }
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("EncodeToJSONValue %v", e.what());
            }
        }

        export pybind11::object GetTypeWithNameInModule(const pybind11::module& m, const std::string& name)
        {
            pybind11::module_ helpers_module = pybind11::module_::import("GiiGaPyHelpers");
            return helpers_module.attr("get_type_with_name_in_module")(m,name);
        }

        export pybind11::object GetBuiltinType(const std::string& name)
        {
            pybind11::module_ builtins = pybind11::module_::import("builtins");
            return builtins.attr(name.c_str());
        }
    }
}
