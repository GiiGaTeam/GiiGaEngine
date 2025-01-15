module;

#include <pybind11/embed.h>

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
import ScriptPropertyModifications;
import IWorldQuery;
import PyBehaviourTrampoline;
import ScriptHelpers;

namespace GiiGa
{
    export class PyBehaviourSchemeComponent : public Component
    {
        //todo: temp?
        friend class ImGuiInspector;

    public:
        PyBehaviourSchemeComponent() = default;

        PyBehaviourSchemeComponent(const PyBehaviourSchemeComponent& other) = delete;
        PyBehaviourSchemeComponent(PyBehaviourSchemeComponent&& other) noexcept = default;
        PyBehaviourSchemeComponent& operator=(const PyBehaviourSchemeComponent& other) = delete;
        PyBehaviourSchemeComponent& operator=(PyBehaviourSchemeComponent&& other) noexcept = default;

        PyBehaviourSchemeComponent(Json::Value json, bool roll_id = false):
            Component(json, roll_id)
        {
            auto script_handle = AssetHandle::FromJson(json["Script"]);

            if (script_handle == AssetHandle())
                return;

            script_asset_ = Engine::Instance().ResourceManager()->GetAsset<ScriptAsset>(script_handle);

            prop_modifications = script_asset_->GetPropertyAnnotaions();

            prop_modifications.SetValuesFromJson(json["PropertyModifications"]);
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result;
            result["Type"] = typeid(PyBehaviourSchemeComponent).name();
            result["Script"] = script_asset_ ? script_asset_->GetId().ToJson() : AssetHandle{}.ToJson();
            
            result["PropertyModifications"] = prop_modifications.toJson();
            
            return result;
        }

        void Init() override
        {
        }

        void BeginPlay() override
        {
            if (!substituter_component_)
            {
                if (auto l_owner = owner_.lock())
                {
                    auto l_owner_go = std::dynamic_pointer_cast<GameObject>(l_owner);
                    substituter_component_ = script_asset_->CreateBehaviourComponent();
                    substituter_component_->RegisterInWorld();
                    l_owner_go->AddComponent(substituter_component_);
                    WorldQuery::AddComponentToBeginPlayQueue(shared_from_this());
                }
            }
            else
            {
                substituter_component_->SetProperties(prop_modifications.prop_modifications);
                this->Destroy();
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

        ::std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> GetPrefabInstanceModifications(::std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const ::GiiGa::PrefabPropertyModifications& modifications) override
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
            prop_modifications = script_asset_->GetPropertyAnnotaions();
            
        }

        std::string GetUserClassName() const
        {
            if (script_asset_)
                return script_asset_->GetUserClassName();
            return "";
        }

        ScriptPropertyModifications& GetPropertyModifications()
        {
            return prop_modifications;
        }

        pybind11::object GetUnderlyingType()
        {
            if (script_asset_)
                return script_asset_->GetUnderlyingType();
            return pybind11::none();
        }

    private:
        ScriptPropertyModifications prop_modifications{};
        std::shared_ptr<ScriptAsset> script_asset_ = nullptr;

        EventHandle<AssetHandle> script_updated_handle_ = EventHandle<AssetHandle>::Null();

        std::shared_ptr<PyBehaviourTrampoline> substituter_component_;

        void OnScriptUpdated(AssetHandle id)
        {
            prop_modifications.MergeWithNewAnnotation(script_asset_->GetPropertyAnnotaions());
        }
    };
}
