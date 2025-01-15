module;

#include <pybind11/embed.h>

export module ScriptPropertyModifications;

import <json/json.h>;

import ScriptHelpers;
import Engine;

namespace GiiGa
{
    export struct PyProperty
    {
        pybind11::type script_type = pybind11::none{};
        pybind11::object value_or_holder = pybind11::none{};

        void Set()
        {
            
        }
    };

    export class ScriptPropertyModifications
    {
    public:
        ScriptPropertyModifications() = default;
        
        ScriptPropertyModifications(const std::unordered_map<std::string, PyProperty>& anot):
            prop_modifications(anot)
        {
            for (auto& [name, prop] : prop_modifications)
            {
                auto opt_holder_type = Engine::Instance().ScriptSystem()->GetReferenceTypeHolder(prop.script_type);
                if (opt_holder_type.has_value())
                {
                    prop.value_or_holder = opt_holder_type.value()();
                }
            }
        }

        void SetValuesFromJson(const Json::Value& json)
        {
            for (const auto& prop_js : json["PropertyModifications"])
            {
                SetProperty(prop_js["Name"].asString(), prop_js["Value"]);
            }
        }

        void SetProperty(std::string name, const Json::Value& json)
        {
            if (prop_modifications.contains(name))
            {
                Json::Reader reader;

                PyProperty& prop = prop_modifications.at(name);

                auto opt_holder_type = Engine::Instance().ScriptSystem()->GetReferenceTypeHolder(prop.script_type);
                if (opt_holder_type.has_value())
                {
                    pybind11::object obj = ScriptHelpers::DecodeFromJSON(json);
                    std::string obj_js_str = pybind11::str(obj);

                    Json::Value root;
                    reader.parse(obj_js_str, root);

                    pybind11::object value = opt_holder_type.value()(root);
                    prop.value_or_holder = value;
                }
                else
                {
                    //todo: list should be reviwed to have ref types 
                    pybind11::object obj = ScriptHelpers::DecodeFromJSON(json);

                    //todo: what if actual dict?
                    if (pybind11::type(obj).is(pybind11::dict{}))
                    {
                        // obj (dict) to json string
                        std::string obj_js_str = pybind11::str(obj);

                        Json::Value root;
                        reader.parse(obj_js_str, root);

                        pybind11::object value = prop.script_type(root);
                        prop.value_or_holder = value;
                    }
                    else
                    {
                        prop.value_or_holder = obj;
                    }
                }
            }
        }

        Json::Value toJson()
        {
            Json::Value result;
            Json::Reader reader;
            for (auto& [name, prop] : prop_modifications)
            {
                Json::Value prop_node;

                prop_node["Name"] = name;

                std::string obj_str = ScriptHelpers::EncodeToJSONStyledString(prop.value_or_holder);

                reader.parse(obj_str, prop_node["Value"]);

                result.append(prop_node);
            }
        }

        void MergeWithNewAnnotation(const std::unordered_map<std::string, PyProperty>& anot)
        {
            ScriptPropertyModifications annotaion_based_modifications(anot);

            for (auto& [name, prop] : annotaion_based_modifications.prop_modifications)
            {
                if (this->prop_modifications.contains(name) &&
                    annotaion_based_modifications.prop_modifications[name].script_type.is(this->prop_modifications[name].script_type))
                {
                    annotaion_based_modifications.prop_modifications[name].value_or_holder = this->prop_modifications[name].value_or_holder;
                }
            }

            this->prop_modifications = annotaion_based_modifications.prop_modifications;
        }

        void clear()
        {
            prop_modifications.clear();
        }

    private:
        std::unordered_map<std::string, PyProperty> prop_modifications{};
    };
}
