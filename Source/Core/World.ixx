module;

#include <string>
#include <vector>
#include <memory>
#include <vector>

export module World;

import Level;
export import GameObject;

namespace GiiGa
{
    export class World
    {
    public:
        World()
        {
            levels_.push_back({});
            levels_[0].Name = "PersistentLevel";
            levels_[0].SetIsActive(true);
        }
        
    public:
        static std::unique_ptr<World>& GetInstance()
        {
            if (instance_) return instance_;
            else return instance_ = std::make_unique<World>();
        }

        static const std::vector<Level>& GetLevels()
        {
            return GetInstance()->levels_;
        }
        
        static void AddLevel(Level&& level)
        {
            GetInstance()->levels_.push_back(level);
        }

        static std::shared_ptr<GameObject> CreateGameObject(std::shared_ptr<Level> Level = nullptr)
        {
            auto obj = std::make_shared<GameObject>();

            if (Level)
            {
                Level->AddGameObject(obj);
            }
            else
            {
                GetInstance()->levels_[0].AddGameObject(obj);
            }

            return obj;
        }

        static bool DestroyGameObject(std::shared_ptr<GameObject> GameObject)
        {
            for (auto level : GetInstance()->levels_)
            {
                if (level.DestroyGameObject(GameObject))
                {
                    return true;
                }
            }
            
            return false;
        }

        static std::shared_ptr<GameObject> Instantiate(GameObject* Prefab,
            std::shared_ptr<GameObject> Parent = nullptr,
            Level* Level = nullptr)
        {
            auto NewGameObject = std::make_shared<GameObject>(*Prefab);
            
            if (Level)
            {
                Level->AddGameObject(NewGameObject);
            }
            else
            {
                GetInstance()->levels_[0].AddGameObject(NewGameObject);
            }

            if (Parent)
            {
                NewGameObject->SetParent(Parent, false);
            }
            
            return NewGameObject;
        }

        static std::shared_ptr<GameObject> Instantiate(GameObject* Prefab,
            std::shared_ptr<GameObject> Parent = nullptr,
            std::string LevelName)
        {
            for (auto level : GetInstance()->levels_)
            {
                if (level.Name == LevelName)
                {
                    return Instantiate(Prefab, Parent, &level);
                }
            }
            
            return nullptr;
        }

    private:
        static inline std::unique_ptr<World> instance_ = nullptr;
        std::vector<Level> levels_;
    };
}