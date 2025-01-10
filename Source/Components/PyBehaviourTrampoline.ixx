module;

#include <pybind11/embed.h>

export module PyBehaviourTrampoline;

import <json/json.h>;

import Component;
import PrefabInstance;
import Logger;

import Misc;

namespace GiiGa
{
    export class PyBehaviourTrampoline : public Component, public pybind11::trampoline_self_life_support
    {
    public:
        using Component::Component;

        void Init() override
        {
            PYBIND11_OVERRIDE_PURE(
                void,             /* Return type */
                GiiGa::Component, /* Parent class */
                Init              /* Name of function in C++ (must match Python name) */
            );
        }

        void Tick(float dt) override
        {
            std::cout << "Tick" << std::endl;
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
                el::Loggers::getLogger(LogPyScript)->debug("ScriptAsset()::CreateBehaviourComponent %v", e.what());
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

        std::vector<std::pair<::GiiGa::PropertyModificationKey, ::GiiGa::PropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const ::GiiGa::PropertyModifications& modifications) override
        {
            Todo();
        }

        ::Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Todo();
            return {};
        }
    };
}
