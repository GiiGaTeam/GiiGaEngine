export module CreateComponentsForGameObject;

import <memory>;
import <json/json.h>;
import <unordered_map>;

import GameObject;
import Component;
import TransformComponent;
import ConsoleComponent;
import StaticMeshComponent;

namespace GiiGa
{
    export struct CreateComponentsForGameObject
    {
        static void Create(std::shared_ptr<GameObject> gameObject, const Json::Value& go_js, bool roll_id = false)
        {
            for (auto&& comp_js : go_js["Components"])
            {
                if (comp_js["Type"].asString() == typeid(TransformComponent).name())
                {
                    gameObject->transform_ = gameObject->CreateComponent<TransformComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(ConsoleComponent).name())
                {
                    gameObject->CreateComponent<ConsoleComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(StaticMeshComponent).name())
                {
                    gameObject->CreateComponent<StaticMeshComponent>(comp_js, roll_id);
                }
            }
        }

        static void Create(std::shared_ptr<GameObject> gameObject, const Json::Value& go_js, std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid)
        {
            bool roll_id = true;
            for (auto&& comp_js : go_js["Components"])
            {
                std::shared_ptr<Component> new_comp;

                if (comp_js["Type"].asString() == typeid(TransformComponent).name())
                {
                    new_comp = gameObject->CreateComponent<TransformComponent>(comp_js, roll_id);
                    gameObject->transform_ = std::dynamic_pointer_cast<TransformComponent>(new_comp);
                }
                else if (comp_js["Type"].asString() == typeid(ConsoleComponent).name())
                {
                    new_comp = gameObject->CreateComponent<ConsoleComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(StaticMeshComponent).name())
                {
                    new_comp = gameObject->CreateComponent<StaticMeshComponent>(comp_js, roll_id);
                }

                prefab_uuid_to_world_uuid[new_comp->GetInPrefabUuid()] = new_comp->GetUuid();
            }
        }
    };
}
