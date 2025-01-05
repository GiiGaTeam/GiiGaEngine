export module ConsoleComponent;

import <iostream>;
import <json/json.h>;
import <optional>;

import Component;
import Time;
import IWorldQuery;
import PrefabModifications;

namespace GiiGa
{
    export class ConsoleComponent final : public Component
    {
    public:
        ConsoleComponent(Json::Value json, bool roll_id = false):
            Component(json, roll_id)
        {
        }

        void Tick(float dt) override
        {
            std::cout << time_sum_ << std::endl;
            time_sum_ += Time::GetDeltaTime();
        }

        void Init() override
        {
        }

        virtual void Restore(const ::Json::Value&) override
        {
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value json;

            json["Type"] = typeid(ConsoleComponent).name();

            json["Properties"] = Json::Value();

            return json;
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid, std::optional<PrefabModifications> modifications) override
        {
            return {};
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
        }

        void RestoreAsPrefab(const Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            
        }

        std::vector<std::pair<PropertyModificationKey,PropertyValue>> GetModifications(std::shared_ptr<IComponent>) const override
        {
            return {};
        }

    private:
        float time_sum_ = 0;
    };
}
