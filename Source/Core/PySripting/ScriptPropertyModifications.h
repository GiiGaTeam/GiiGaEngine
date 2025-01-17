#pragma once
#include <pybind11/embed.h>


#include<json/json.h>

#include<ScriptHelpers.h>
#include<Engine.h>
#include<Logger.h>
#include<Misc.h>
#include<Uuid.h>

namespace GiiGa
{
    struct PyProperty
    {
        pybind11::type script_type = pybind11::none{};
        pybind11::object value_or_holder = pybind11::none{};

        void Set(const Json::Value& json)
        {
            try
            {
                pybind11::object value_type = pybind11::type::of(value_or_holder);                
                if (value_type.is(ScriptHelpers::GetBuiltinType("int")))
                {
                    value_or_holder = pybind11::cast(json.asInt());
                    return;
                }
                else if (value_type.is(ScriptHelpers::GetBuiltinType("bool")))
                {
                    value_or_holder = pybind11::cast(json.asBool());
                    return;
                }
                else if (value_type.is(ScriptHelpers::GetBuiltinType("str")))
                {
                    value_or_holder = pybind11::cast(json.asString());
                    return;
                }
                else if (value_type.is(ScriptHelpers::GetBuiltinType("float")))
                {
                    value_or_holder = pybind11::cast(json.asFloat());
                    return;
                }
                else if (value_type.is(ScriptHelpers::GetBuiltinType("list")))
                {
                    if (json.isArray())
                    {
                        Todo();
                    }
                }
                else if (value_type.is(pybind11::type::of<Uuid>()))
                {
                    if (json.isString())
                    {
                        value_or_holder = pybind11::cast(Uuid::FromString(json.asString()).value());
                    }
                }
                else
                {
                    if (json.isObject())
                    {
                        value_or_holder = pybind11::type::of(value_or_holder)(json);
                        return;
                    }
                    Todo();
                }
                throw std::runtime_error("Invalid value for py script property");
            }
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->info("value_or_holder = value_or_holder.get_type()(json) %v", e.what());
            }
        }
    };

    class ScriptPropertyModifications
    {
        friend class PyBehaviourSchemeComponent;

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
                prop_modifications.at(name).Set(json);
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

                result.append(ScriptHelpers::EncodeToJSONValue(prop.value_or_holder));
            }
            return result;
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

        class Iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = std::pair<const std::string, PyProperty>;
            using pointer = value_type*;
            using reference = value_type&;

            Iterator(std::unordered_map<std::string, PyProperty>::iterator itr) : itr_(itr)
            {
            }

            reference operator*() const { return *itr_; }
            pointer operator->() { return &(*itr_); }

            // Prefix increment
            Iterator& operator++()
            {
                itr_++;
                return *this;
            }

            // Postfix increment
            Iterator operator++(int)
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const Iterator& a, const Iterator& b) { return a.itr_ == b.itr_; }
            friend bool operator!=(const Iterator& a, const Iterator& b) { return a.itr_ != b.itr_; }

        private:
            std::unordered_map<std::string, PyProperty>::iterator itr_;
        };

        Iterator begin() { return Iterator(prop_modifications.begin()); }
        Iterator end() { return Iterator(prop_modifications.end()); }

    private:
        std::unordered_map<std::string, PyProperty> prop_modifications{};
    };
}
