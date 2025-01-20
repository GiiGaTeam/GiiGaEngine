#pragma once
#include<iostream>
#include<json/json.h>
#include<optional>

#include<Component.h>
#include<Timer.h>
#include<PrefabInstanceModifications.h>

namespace GiiGa
{
    class ConsoleComponent final : public Component
    {
    public:
        ConsoleComponent(Json::Value json, bool roll_id = false):
            Component(json, roll_id)
        {
        }

        void Tick(float dt) override
        {
            std::cout << time_sum_ << std::endl;
            time_sum_ += Timer::GetDeltaTime();
        }

        void Init() override
        {
        }
        
        void BeginPlay() override
        {
            
        }

        virtual void RestoreFromLevelJson(const ::Json::Value&) override
        {
        }

        Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value json;

            json["Type"] = typeid(ConsoleComponent).name();

            json["Properties"] = Json::Value();

            return json;
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid, const std::optional<std::unordered_map<Uuid, Uuid>>
                                          & instance_uuid) override
        {
            return {};
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
        }

        void RestoreAsPrefab(const Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
            
        }

        std::vector<std::pair<PropertyModificationKey,PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            return {};
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications) override
        {
        }


    private:
        float time_sum_ = 0;
    };
}
