export module PyBehaviourSchemeComponent;

import <unordered_map>;
import <optional>;
import <memory>;
import <json/json.h>;

import PrefabInstance;
import Component;
import Misc;
import ScriptAsset;
import AssetHandle;
import Engine;

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
            script_asset_ = Engine::Instance().ResourceManager()->GetAsset<ScriptAsset>(handle);
        }

    private:
        std::unordered_map<std::string, Json::Value> prop_modifications{};
        std::shared_ptr<ScriptAsset> script_asset_ = nullptr;
    };
}
