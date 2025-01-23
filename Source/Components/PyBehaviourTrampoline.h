#pragma once
#include <pybind11/embed.h>

#include<json/json.h>

#include<Component.h>
#include<PrefabInstanceModifications.h>
#include<Logger.h>
#include<Engine.h>
#include<ScriptPropertyModifications.h>
#include<IWorldQuery.h>

#include<Misc.h>

namespace GiiGa
{
    class PyBehaviourTrampoline : public Component, public pybind11::trampoline_self_life_support
    {
        friend class ScriptAsset;

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
            catch (pybind11::error_already_set& e)
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
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("PyBehaviourTrampoline::BeginPlay %v", e.what());
            }
            catch (std::exception& e)
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
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("PyBehaviourTrampoline::Tick %v", e.what());
            }
        }

        void OnBeginOverlap(const std::shared_ptr<CollisionComponent>& other_comp, const CollideInfo& collideInfo) override
        {
            try
            {
                PYBIND11_OVERRIDE(
                    void,
                    GiiGa::Component,
                    OnBeginOverlap,
                    other_comp,
                    collideInfo
                );
            }
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("PyBehaviourTrampoline::Tick %v", e.what());
            }
        }

        void OnOverlapping(const std::shared_ptr<CollisionComponent>& other_comp, const CollideInfo& collideInfo) override
        {
            try
            {
                PYBIND11_OVERRIDE(
                    void,
                    GiiGa::Component,
                    OnOverlapping,
                    other_comp,
                    collideInfo
                );
            }
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("PyBehaviourTrampoline::Tick %v", e.what());
            }
        }

        void OnEndOverlap(const std::shared_ptr<CollisionComponent>& other_comp) override
        {
            try
            {
                PYBIND11_OVERRIDE(
                    void,
                    GiiGa::Component,
                    OnEndOverlap,
                    other_comp
                );
            }
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("PyBehaviourTrampoline::Tick %v", e.what());
            }
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid, const std::optional<std::unordered_map<Uuid, Uuid>>& instance_uuid) override
        {
            Todo();
            return {};
        }

        void RestoreFromLevelJson(const ::Json::Value&) override
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
                    Uuid uuid = pybind11::cast<Uuid>(prop.value_or_holder);
                    if (uuid == Uuid::Null())
                        continue;
                    std::shared_ptr<IGameObject> value = WorldQuery::GetWithUUID<IGameObject>(uuid);
                    setattr(pybind11::cast(this),
                            pybind11::cast(name),
                            pybind11::cast(value, pybind11::return_value_policy::reference));
                }
                else if (Engine::Instance().ScriptSystem()->IsTypeComponent(prop.script_type))
                {
                    Uuid uuid = pybind11::cast<Uuid>(prop.value_or_holder);
                    if (uuid == Uuid::Null())
                        continue;
                    std::shared_ptr<IComponent> value = WorldQuery::GetWithUUID<IComponent>(uuid);
                    setattr(pybind11::cast(this),
                            pybind11::cast(name),
                            pybind11::cast(value));
                }
                else
                    setattr(pybind11::cast(this),
                            pybind11::cast(name),
                            prop.value_or_holder);
            }
        }
    };
}
