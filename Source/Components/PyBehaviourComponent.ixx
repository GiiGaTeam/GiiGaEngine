export module PyBehaviourComponent;

import <unordered_map>;
import <optional>;
import <memory>;
import <json/json.h>;

import PrefabInstance;
import Component;
import Misc;

namespace GiiGa
{
    export class PyBehaviourComponent : public Component
    {
    public:
        PyBehaviourComponent(Json::Value json, bool roll_id = false):
            Component(json, roll_id)
        {
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value result;
            return result;
        }

        void Init() override
        {
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

        ::std::vector<std::pair<PropertyModificationKey, PropertyValue>> GetModifications(::std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const ::GiiGa::PropertyModifications& modifications) override
        {
            Todo();
        }

    private:
    };
}
