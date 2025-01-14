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
import PyProperty;
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

            prop_modifications = script_asset_->GetPropertieAnnotaions();

            Json::CharReaderBuilder builder;
            Json::CharReader* reader = builder.newCharReader();

            for (const auto& prop_js : json["PropertyModifications"])
            {
                auto prop_it = prop_modifications.find(prop_js["Name"].asString());
                if (prop_it != prop_modifications.end())
                {
                    auto opt_holder_type = Engine::Instance().ScriptSystem()->GetReferenceTypeHolder(prop_it->second.script_type);
                    if (!opt_holder_type.has_value())
                    {
                        pybind11::object obj = ScriptHelpers::DecodeFromJSON(prop_js["Value"]);
                        std::string obj_js_str = pybind11::str(obj);
                        Json::Value root;
                        reader->parse(
                            obj_js_str.c_str(),
                            obj_js_str.c_str() + obj_js_str.size(),
                            &root,
                            nullptr
                        );
                        pybind11::object value = opt_holder_type.value()(pybind11::cast(root));
                        prop_it->second.value_or_holder = value;
                    }
                    else
                    {
                        //todo: list should be reviwed to have ref types 
                        pybind11::object obj = ScriptHelpers::DecodeFromJSON(prop_js["Value"]);

                        //todo: what if actual dict?
                        if (pybind11::type(obj).is(pybind11::dict{}))
                        {
                            // obj (dict) to json string
                            std::string obj_js_str = pybind11::str(obj);
                            Json::Value root;
                            reader->parse(
                                obj_js_str.c_str(),
                                obj_js_str.c_str() + obj_js_str.size(),
                                &root,
                                nullptr
                            );
                            pybind11::object value = prop_it->second.script_type(pybind11::cast(root));
                            prop_it->second.value_or_holder = value;
                        }
                        else
                        {
                            prop_it->second.value_or_holder = obj;
                        }
                    }
                }
            }

            delete reader;
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result;
            result["Type"] = typeid(PyBehaviourSchemeComponent).name();
            result["Script"] = script_asset_ ? script_asset_->GetId().ToJson() : AssetHandle{}.ToJson();

            Json::CharReaderBuilder builder;
            Json::CharReader* reader = builder.newCharReader();

            for (auto& [name, prop] : prop_modifications)
            {
                Json::Value prop_node;

                prop_node["Name"] = name;

                std::string obj_str = ScriptHelpers::EncodeToJSONStyledString(prop.value_or_holder);

                reader->parse(
                    obj_str.c_str(),
                    obj_str.c_str() + obj_str.size(),
                    &prop_node["Value"],
                    nullptr
                );

                result["PropertyModifications"].append(prop_node);
            }

            delete reader;

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
                substituter_component_->SetProperties(prop_modifications);
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

        std::shared_ptr<PyBehaviourTrampoline> substituter_component_;

        void OnScriptUpdated(AssetHandle id)
        {
            //todo: find way to merge
            prop_modifications.clear();
            prop_modifications = script_asset_->GetPropertieAnnotaions();
        }
    };
}
