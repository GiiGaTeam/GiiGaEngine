#pragma once


#include<memory>
#include<json/json.h>
#include<unordered_map>

#include<GameObject.h>
#include<Component.h>
#include<TransformComponent.h>
#include<ConsoleComponent.h>
#include<StaticMeshComponent.h>
#include<PyBehaviourSchemeComponent.h>
#include<DirectionalLightComponent.h>
#include <PointLightComponent.h>

namespace GiiGa
{
    struct CreateComponentsForGameObject
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
                else if (comp_js["Type"].asString() == typeid(PyBehaviourSchemeComponent).name())
                {
                    gameObject->CreateComponent<PyBehaviourSchemeComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(DirectionalLightComponent).name())
                {
                    gameObject->CreateComponent<DirectionalLightComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(PointLightComponent).name())
                {
                    gameObject->CreateComponent<PointLightComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(CameraComponent).name())
                {
                    gameObject->CreateComponent<CameraComponent>(comp_js, roll_id);
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
                else if (comp_js["Type"].asString() == typeid(PyBehaviourSchemeComponent).name())
                {
                    gameObject->CreateComponent<PyBehaviourSchemeComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(DirectionalLightComponent).name())
                {
                    gameObject->CreateComponent<DirectionalLightComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(PointLightComponent).name())
                {
                    gameObject->CreateComponent<PointLightComponent>(comp_js, roll_id);
                }
                else if (comp_js["Type"].asString() == typeid(CameraComponent).name())
                {
                    gameObject->CreateComponent<CameraComponent>(comp_js, roll_id);
                }

                prefab_uuid_to_world_uuid[new_comp->GetInPrefabUuid()] = new_comp->GetUuid();
            }
        }
    };
}
