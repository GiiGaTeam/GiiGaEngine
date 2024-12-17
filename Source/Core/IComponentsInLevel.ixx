module;

#include <unordered_map>
#include <memory>
#include <typeindex>

export module IComponentsInLevel;

import IComponent;

namespace GiiGa
{
    export class IComponentsInLevel
    {
    public:
        virtual ~IComponentsInLevel()
        {
        }

        template <typename T>
        void AddComponent(std::shared_ptr<T> component)
        {
            static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from Component");
            type_to_components_[typeid(*component)].push_back(component);
        }

        void RemoveComponent(const std::shared_ptr<IComponent>& component)
        {
            // Ѕудет работать только в том случае, если умные указатели будут корректно сравниватьс€ в течении всей работы программы.
            // Ќа короткой дистанции так скорее всего и будет.

            std::type_index typeIndex(typeid(*component));
            auto it = type_to_components_.find(typeIndex);
            if (it != type_to_components_.end())
            {
                auto& vec = it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), component), vec.end());
            }
        }

    protected:
        std::unordered_map<std::type_index, std::vector<std::shared_ptr<IComponent>>> type_to_components_;
    };
}
