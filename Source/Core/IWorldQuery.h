#pragma once


#include<unordered_map>
#include<memory>
#include<typeindex>
#include<any>
#include<stdexcept>
#include<queue>

#include<IComponent.h>
#include<Uuid.h>
#include<Logger.h>
#include<ILevelRootGameObjects.h>

namespace GiiGa
{
    class WorldQuery
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

        static void AddAnyWithUuid(const Uuid& uuid, std::shared_ptr<void> value)
        {
            auto& instance = GetInstance();
            el::Loggers::getLogger(LogWorldQuery)->debug("Registering any with uuid %v", uuid.ToString());
            if (instance.uuid_to_any_.contains(uuid))
                throw std::runtime_error("Failed to AddAnyWithUuid, duplicated uuid!");

            instance.uuid_to_any_.insert({uuid, value});
        }

        static void RemoveAnyWithUuid(const Uuid& uuid)
        {
            if (uuid == Uuid::Null())
                return;

            if (!GetInstance().uuid_to_any_.contains(uuid))
                throw std::runtime_error("Failed to RemoveAnyWithUuid, not found uuid!");
            el::Loggers::getLogger(LogWorldQuery)->debug("RemoveAnyWithUuid any with uuid %v", uuid.ToString());
            GetInstance().uuid_to_any_.erase(uuid);
        }

        template <typename T>
        static std::shared_ptr<T> GetWithUUID(const Uuid& uuid)
        {
            auto inst = GetInstance();

            if (!inst.uuid_to_any_.contains(uuid))
                throw std::runtime_error("Failed to GetWithUUID, not found uuid!");

            auto any = inst.uuid_to_any_.at(uuid);

            std::shared_ptr<void> shared_ptr = nullptr;
            try
            {
                auto weak_ptr = std::any_cast<std::weak_ptr<void>>(any);
                if (weak_ptr.expired())
                    throw std::runtime_error("Failed to get any of any with uuid! Something went wrong");
                shared_ptr = weak_ptr.lock();
            }
            catch (const std::exception& e)
            {
                el::Loggers::getLogger(LogWorldQuery)->fatal("Failed to get any of type %v", e.what());
            }

            return std::static_pointer_cast<T>(shared_ptr);
        }

        template <typename T>
        static void AddComponent(std::shared_ptr<T> component)
        {
            static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from Component");
            GetInstance().comp_init_queue_.push(component);
            //GetInstance().type_to_components_[typeid(*component)].push_back(component);
        }

        static void AddComponentToBeginPlayQueue(std::shared_ptr<IComponent> component)
        {
            GetInstance().comp_begin_play_queue_.push(component);
        }

        static void RemoveComponent(IComponent* component)
        {
            // ����� �������� ������ � ��� ������, ���� ����� ��������� ����� ��������� ������������ � ������� ���� ������ ���������.
            // �� �������� ��������� ��� ������ ����� � �����.

            /*
            std::type_index typeIndex(typeid(*component));
            auto it = GetInstance().type_to_components_.find(typeIndex);
            if (it != GetInstance().type_to_components_.end())
            {
                auto& vec = it->second;
                auto where = std::find_if(vec.begin(), vec.end(), [component](std::weak_ptr<IComponent> c)
                {
                    if (auto l_c = c.lock())
                    {
                        return l_c.get() == component;
                    }
                    else
                    {
                        throw std::runtime_error("Failed to RemoveComponent, weak ref expired");
                    }
                });
                vec.erase(where);
            }
            */
        }

        template <typename T>
        static std::vector<std::weak_ptr<T>> GetComponentsOfType()
        {
            static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from Component");

            std::vector<std::weak_ptr<T>> result;

            for (const auto& [uuid, some] : GetInstance().uuid_to_any_)
            {
                if (auto castedComponent = std::dynamic_pointer_cast<T>(std::static_pointer_cast<IComponent>(some.lock())))
                {
                    result.push_back(castedComponent);
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
        //std::unordered_map<std::type_index, std::vector<std::weak_ptr<IComponent>>> type_to_components_;
        std::unordered_map<Uuid, std::weak_ptr<void>> uuid_to_any_;
        std::queue<std::weak_ptr<IComponent>> comp_init_queue_;
        std::queue<std::weak_ptr<IComponent>> comp_begin_play_queue_;

        virtual std::shared_ptr<ILevelRootGameObjects> GetPersistentLevel_Impl()
        {
            throw std::runtime_error("AttachGameObjectToPersistentLevel_Impl");
            return nullptr;
        }
    };
}
