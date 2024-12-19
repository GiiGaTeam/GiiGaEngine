module;

#include <unordered_map>
#include <memory>
#include <typeindex>
#include <any>
#include <stdexcept>
#include <queue>

export module IWorldQuery;

import IComponent;
import IGameObject;
import Uuid;
import Logger;
export import ILevelRootGameObjects;

namespace GiiGa
{
    export class WorldQuery
    {
    public:
        virtual ~WorldQuery() = default;

        static WorldQuery& GetInstance()
        {
            if (instance_)
                return *instance_;
            else
            {
                el::Loggers::getLogger(LogWorld)->fatal("Failed to get instance of WorldQuery, Call To World Init first!");
                throw std::runtime_error("Failed to get instance of WorldQuery, Call To World Init first!");
            }
        }

        static void AddAnyWithUuid(const Uuid& uuid, std::any value)
        {
            GetInstance().uuid_to_any_.insert({uuid, value});
        }

        static void RemoveAnyWithUuid(const Uuid& uuid)
        {
            GetInstance().uuid_to_any_.erase(uuid);
        }

        template <typename T>
        static T GetWithUUID(const Uuid& uuid)
        {
            return std::any_cast<T>(GetInstance().uuid_to_any_.at(uuid));
        }

        template <typename T>
        static void AddComponent(std::shared_ptr<T> component)
        {
            static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from Component");
            GetInstance().comp_init_queue_.push(component);
            GetInstance().type_to_components_[typeid(*component)].push_back(component);
        }

        static void RemoveComponent(const std::shared_ptr<IComponent>& component)
        {
            // Ѕудет работать только в том случае, если умные указатели будут корректно сравниватьс€ в течении всей работы программы.
            // Ќа короткой дистанции так скорее всего и будет.

            std::type_index typeIndex(typeid(*component));
            auto it = GetInstance().type_to_components_.find(typeIndex);
            if (it != GetInstance().type_to_components_.end())
            {
                auto& vec = it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), component), vec.end());
            }
        }

        template <typename T>
        static std::vector<std::shared_ptr<T>> getComponentsOfType()
        {
            static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from Component");

            std::type_index typeIndex(typeid(T));
            std::vector<std::shared_ptr<T>> result;

            auto it = GetInstance().type_to_components_.find(typeIndex);
            if (it != GetInstance().type_to_components_.end())
            {
                for (const auto& component : it->second)
                {
                    if (auto castedComponent = std::dynamic_pointer_cast<T>(component))
                    {
                        result.push_back(castedComponent);
                    }
                }
            }

            return result;
        }

        static std::shared_ptr<ILevelRootGameObjects> GetPersistentLevel()
        {
            return GetInstance().GetPersistentLevel_Impl();
        }

    protected:
        static inline std::shared_ptr<WorldQuery> instance_;
        std::unordered_map<std::type_index, std::vector<std::shared_ptr<IComponent>>> type_to_components_;
        std::unordered_map<Uuid, std::any> uuid_to_any_;

        std::queue<std::shared_ptr<IComponent>> comp_init_queue_;

        virtual std::shared_ptr<ILevelRootGameObjects> GetPersistentLevel_Impl()
        {
            throw std::runtime_error("AttachGameObjectToPersistentLevel_Impl");
            return nullptr;
        }
    };
}
