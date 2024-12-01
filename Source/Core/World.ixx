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
            levels_[0].name = "PersistentLevel";
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

        static std::shared_ptr<GameObject> CreateGameObject(std::shared_ptr<Level> level = nullptr)
        {
            auto obj = std::make_shared<GameObject>();

            if (level)
            {
                level->AddGameObject(obj);
            }
            else
            {
                GetInstance()->levels_[0].AddGameObject(obj);
            }

            return obj;
        }

        static bool DestroyGameObject(std::shared_ptr<GameObject> gameObject)
        {
            for (auto level : GetInstance()->levels_)
            {
                if (level.DestroyGameObject(gameObject))
                {
                    return true;
                }
            }
            
            return false;
        }

        static std::shared_ptr<GameObject> Instantiate(GameObject* prefab,
            std::shared_ptr<GameObject> parent = nullptr,
            Level* level = nullptr)
        {
            auto newGameObject = prefab->Clone();
            
            if (level)
            {
                level->AddGameObject(newGameObject);
            }
            else
            {
                GetInstance()->levels_[0].AddGameObject(newGameObject);
            }

            if (parent)
            {
                newGameObject->SetParent(parent, false);
            }
            
            return newGameObject;
        }

        static std::shared_ptr<GameObject> Instantiate(GameObject* prefab,
            std::shared_ptr<GameObject> parent = nullptr,
            std::string levelName)
        {
            for (auto level : GetInstance()->levels_)
            {
                if (level.name == levelName)
                {
                    return Instantiate(prefab, parent, &level);
                }
            }
            
            return nullptr;
        }

    private:
        static inline std::unique_ptr<World> instance_ = nullptr;
        std::vector<Level> levels_;
    };
}