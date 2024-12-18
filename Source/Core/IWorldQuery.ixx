module;

#include <unordered_map>
#include <memory>
#include <typeindex>
#include <any>

export module IWorldQuery;

import IComponent;
import Uuid;

namespace GiiGa
{
    export class WorldQuery final
    {
    public:

        static std::unique_ptr<WorldQuery>& GetInstance()
        {
            if (instance_) return instance_;
            else return instance_ = std::make_unique<WorldQuery>();
        }
        
        static void AddAnyWithUuid(const Uuid& uuid, std::any value)
        {
            GetInstance()->uuid_to_any_.insert({uuid, value});
        }

        static void RemoveAnyWithUuid(const Uuid& uuid)
        {
            GetInstance()->uuid_to_any_.erase(uuid);
        }

        template <typename T>
        static T GetWithUUID(const Uuid& uuid)
        {
            return std::any_cast<T>(GetInstance()->uuid_to_any_.at(uuid));
        }

        template <typename T>
        static void AddComponent(std::shared_ptr<T> component)
        {
            static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from Component");
            GetInstance()->type_to_components_[typeid(*component)].push_back(component);
        }

        static void RemoveComponent(const std::shared_ptr<IComponent>& component)
        {
            // Ѕудет работать только в том случае, если умные указатели будут корректно сравниватьс€ в течении всей работы программы.
            // Ќа короткой дистанции так скорее всего и будет.

            std::type_index typeIndex(typeid(*component));
            auto it = GetInstance()->type_to_components_.find(typeIndex);
            if (it != GetInstance()->type_to_components_.end())
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

            auto it = GetInstance()->type_to_components_.find(typeIndex);
            if (it != GetInstance()->type_to_components_.end())
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

    protected:
        static inline std::unique_ptr<WorldQuery> instance_ = nullptr;
        std::unordered_map<std::type_index, std::vector<std::shared_ptr<IComponent>>> type_to_components_;
        std::unordered_map<Uuid, std::any> uuid_to_any_;
    };
}
