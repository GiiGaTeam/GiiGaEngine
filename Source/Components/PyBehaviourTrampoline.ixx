module;

#include <pybind11/embed.h>

export module PyBehaviourTrampoline;

import <json/json.h>;

import Component;
import PrefabInstance;
import Logger;
import Engine;
import ScriptPropertyModifications;
import IWorldQuery;

import Misc;

namespace GiiGa
{
    export class PyBehaviourTrampoline : public Component, public pybind11::trampoline_self_life_support
    {
    public:
        using Component::Component;

        void Init() override
        {
            try
            {
                PYBIND11_OVERRIDE_PURE(
                    void,             /* Return type */
                    GiiGa::Component, /* Parent class */
                    Init              /* Name of function in C++ (must match Python name) */
                );
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("PyBehaviourTrampoline::Init %v", e.what());
            }
        }

        void BeginPlay() override
        {
            try
            {
                PYBIND11_OVERRIDE_PURE(
                    void,             /* Return type */
                    GiiGa::Component, /* Parent class */
                    BeginPlay         /* Name of function in C++ (must match Python name) */
                );
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("PyBehaviourTrampoline::BeginPlay %v", e.what());
            }
        }

        void Tick(float dt) override
        {
            try
            {
                PYBIND11_OVERRIDE_PURE(
                    void,
                    GiiGa::Component,
                    Tick,
                    dt
                );
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("PyBehaviourTrampoline::Tick %v", e.what());
            }
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid, const std::optional<std::unordered_map<Uuid, Uuid>>& instance_uuid) override
        {
            Todo();
            return {};
        }

        void Restore(const ::Json::Value&) override
        {
            Todo();
        }

        void RestoreAsPrefab(const ::Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        std::vector<std::pair<::GiiGa::PropertyModificationKey, ::GiiGa::PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const ::GiiGa::PrefabPropertyModifications& modifications) override
        {
            Todo();
        }

        ::Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Todo();
            return {};
        }

        void SetProperties(const std::unordered_map<std::string, PyProperty>& properties)
        {
            for (const auto& [name, prop] : properties)
            {
                // ugly as f
                if (Engine::Instance().ScriptSystem()->IsTypeGameObject(prop.script_type))
                {
                    if (prop.value_or_holder.is(pybind11::none()))
                        continue;
                    Uuid uuid = pybind11::cast<Uuid>(prop.value_or_holder);
                    std::shared_ptr<IGameObject> value = WorldQuery::GetWithUUID<IGameObject>(uuid);
                    setattr(pybind11::cast(this),
                            pybind11::cast(name),
                            pybind11::cast(value));
                }
                if (Engine::Instance().ScriptSystem()->IsTypeComponent(prop.script_type))
                {
                    if (prop.value_or_holder.is(pybind11::none()))
                        continue;
                    Uuid uuid = pybind11::cast<Uuid>(prop.value_or_holder);
                    std::shared_ptr<IComponent> value = WorldQuery::GetWithUUID<IComponent>(uuid);
                    setattr(pybind11::cast(this),
                            pybind11::cast(name),
                            pybind11::cast(value));
                }
                setattr(pybind11::cast(this),
                        pybind11::cast(name),
                        prop.value_or_holder);
            }
        }
    };
}
