module;

#include <pybind11/pybind11.h>

export module PyBehaviourSchemeComponent;

import <unordered_map>;
import <optional>;
import <memory>;
import <json/json.h>;
import <any>;

import PrefabInstance;
import Component;
import Misc;
import ScriptAsset;
import AssetHandle;
import Engine;
import GameObject;
import Logger;
import EventSystem;
import PyProperty;

namespace GiiGa
{    
    export class PyBehaviourSchemeComponent : public Component
    {
        //todo: temp?
        friend class ImGuiInspector;

    public:
        PyBehaviourSchemeComponent() = default;

        PyBehaviourSchemeComponent(Json::Value json, bool roll_id = false):
            Component(json, roll_id)
        {
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result;
            result["Type"] = typeid(PyBehaviourSchemeComponent).name();
            return result;
        }

        void Init() override
        {
        }

        void BeginPlay() override
        {
            if (auto l_owner = owner_.lock())
            {
                auto l_owner_go = std::dynamic_pointer_cast<GameObject>(l_owner);
                auto new_comp = script_asset_->CreateBehaviourComponent();
                new_comp->RegisterInWorld();
                l_owner_go->AddComponent(new_comp);
            }
        }

        void Restore(const Json::Value&) override
        {
            Todo();
        }

        void RestoreAsPrefab(const Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        void Tick(float dt) override
        {
        }

        ::std::shared_ptr<IComponent> Clone(::std::unordered_map<Uuid, Uuid>& original_uuid_to_world_uuid, const ::std::optional<::std::unordered_map<Uuid, Uuid>>& instance_uuid) override
        {
            Todo();
            return {};
        }

        void RestoreFromOriginal(::std::shared_ptr<IComponent> original, const ::std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            Todo();
        }

        ::std::vector<std::pair<PropertyModificationKey, PropertyValue>> GetPrefabInstanceModifications(::std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const ::GiiGa::PropertyModifications& modifications) override
        {
            Todo();
        }

        //temp:
        AssetHandle GetScriptHandle()
        {
            if (script_asset_)
                return script_asset_->GetId();
            else
                return {};
        }

        void SetScriptHandle(const AssetHandle& handle)
        {
            if (script_asset_)
                script_asset_->OnUpdate.Unregister(script_updated_handle_);

            script_asset_ = Engine::Instance().ResourceManager()->GetAsset<ScriptAsset>(handle);
            script_asset_->OnUpdate.Register(std::bind(&PyBehaviourSchemeComponent::OnScriptUpdated, this, std::placeholders::_1));

            prop_modifications.clear();
            prop_modifications = script_asset_->GetPropertieAnnotaions();
        }

        std::string GetUserClassName() const
        {
            if (script_asset_)
                return script_asset_->GetUserClassName();
            return "";
        }

    private:
        std::unordered_map<std::string, PyProperty> prop_modifications{};
        std::shared_ptr<ScriptAsset> script_asset_ = nullptr;

        EventHandle<AssetHandle> script_updated_handle_ = EventHandle<AssetHandle>::Null();

        void OnScriptUpdated(AssetHandle id)
        {
            //todo: find way to merge
            prop_modifications.clear();
            prop_modifications = script_asset_->GetPropertieAnnotaions();
        }
    };
}
