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

        export std::string EncodeToJSONStyledString(const pybind11::object& obj)
        {
            pybind11::module_ helpers_module = pybind11::module_::import("GiiGaPyHelpers");
            try
            {
                pybind11::object t = helpers_module.attr("EncodeToJSONStyledString")(obj);
                return t.cast<std::string>();
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("EncodeToJSONStyledString %v", e.what());
            }
        }

        // returns simple type (str, int...) or dict if is object
        export pybind11::object DecodeFromJSON(const Json::Value& js)
        {
            pybind11::object s = pybind11::str(js.toStyledString());
            pybind11::module_ helpers_module = pybind11::module_::import("GiiGaPyHelpers");
            return helpers_module.attr("DecodeFromJSON")(s);
        }
    }
}
